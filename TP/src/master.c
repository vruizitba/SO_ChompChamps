
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include "master.h"
#include <sys/wait.h>
#include <util.h>
#include <unistd.h>
#include <sys/select.h>

#include "sync.h"
#include "common.h"

void inline static set_valid_positions(game_state_t* gs, int player_pos);
void inline static writer_lock(sync_t* s);
void inline static writer_unlock(sync_t* s);
bool inline static is_valid_move(int player_pos, int new_x, int new_y , game_state_t* gs);
bool inline static free_cell(int cell_value);
void inline static wait_all(game_state_t* gs, pid_t view);
void inline static print_winners(game_state_t* gs);
void close_fds(int fds[][2], int num_players);

void close_fds(int fds[][2], int num_players) {
    for (int i = 0; i < num_players; i++) {
        close(fds[i][0]); // Cierra el extremo de lectura
    }
}

void inline static print_winners(game_state_t* gs) {
    int best_score = 0;
    int best_players[MAX_PLAYERS], count = 0;

    for (unsigned int i = 0; i < gs->num_players; i++) {
        if (gs->players[i].score > best_score) {
            // New best score, reset the list of best players
            best_score = gs->players[i].score;
            count = 0;
            best_players[count++] = i;
        }
        else if (gs->players[i].score == best_score) {
            // Same score, add to the list
            best_players[count++] = i;
        }
    }

    // If there's only one winner by score
    if (count == 1) {
        printf("Winner: %s (Score: %u)\n", gs->players[best_players[0]].name, gs->players[best_players[0]].score);
        return;
    }

    // Tiebreaker by valid moves
    int min_valids = __INT_MAX__;
    for (int i = 0; i < count; i++) {
        if (gs->players[best_players[i]].valids < min_valids)
            min_valids = gs->players[best_players[i]].valids;
    }

    int tied_by_valids[MAX_PLAYERS], tied_count = 0;
    for (int i = 0; i < count; i++) {
        if (gs->players[best_players[i]].valids == min_valids)
            tied_by_valids[tied_count++] = best_players[i];
    }

    // If there's only one winner by valid moves
    if (tied_count == 1) {
        printf("Winner: %s (Score: %u, Valids: %u)\n",
               gs->players[tied_by_valids[0]].name,
               gs->players[tied_by_valids[0]].score,
               gs->players[tied_by_valids[0]].valids);
        return;
    }

    // Tiebreaker by invalid moves
    int min_invalids = __INT_MAX__;
    int final_winners[MAX_PLAYERS], final_count = 0;

    for (int i = 0; i < tied_count; i++) {
        int player_idx = tied_by_valids[i];
        int player_invalids = gs->players[player_idx].invalids;

        if (player_invalids < min_invalids) {
            // New minimum found, reset the list
            min_invalids = player_invalids;
            final_count = 0;
            final_winners[final_count++] = player_idx;
        }
        else if (player_invalids == min_invalids) {
            // Equal to current minimum, add to the list
            final_winners[final_count++] = player_idx;
        }
    }

    // If there's only one winner after all tiebreakers
    if (final_count == 1) {
        printf("Winner: %s (Score: %u, Valids: %u, Invalids: %u)\n",
               gs->players[final_winners[0]].name,
               gs->players[final_winners[0]].score,
               gs->players[final_winners[0]].valids,
               gs->players[final_winners[0]].invalids);
        return;
    }

    // Tie between multiple players
    printf("Tie between:\n");
    for (int i = 0; i < final_count; i++) {
        int idx = final_winners[i];
        printf("- %s (Score: %u, Valids: %u, Invalids: %u)\n",
               gs->players[idx].name,
               gs->players[idx].score,
               gs->players[idx].valids,
               gs->players[idx].invalids);
    }
}

void inline static wait_all(game_state_t* gs, pid_t view) {
    int status;
    for (int i = 0; i < gs->num_players; i++) {
        waitpid(gs->players[i].pid, &status, 0);

        printf("%s (PID %d) - Score: %u, Valids: %u, Invalids: %u, Exit code: %d\n", gs->players[i].name, gs->players[i].pid, gs->players[i].score, gs->players[i].valids, gs->players[i].invalids, status);
    }

    waitpid(view, &status, 0);
}

void inline static set_valid_positions(game_state_t* gs, int player_pos) {
    int x, y;
    do {
        x = rand() % gs->width;
        y = rand() % gs->height;
    } while (!free_cell(gs->board[x + y * gs->width]));
    gs->players[player_pos].x = x;
    gs->players[player_pos].y = y;
    gs->board[x + y * gs->width] = -player_pos;
}

bool inline static free_cell(int cell_value) {
    return (cell_value > 0 && cell_value <= 9);
}

void inline static writer_lock(sync_t* s) {
    sem_wait(&s->accessor_queue_signal);
    sem_wait(&s->full_access_signal);
}

void inline static writer_unlock(sync_t* s) {
    sem_post(&s->full_access_signal);
    sem_post(&s->accessor_queue_signal);
}

bool inline static is_valid_move(int player_pos, int new_x, int new_y, game_state_t* gs) {

    return (new_x >= 0 && new_x < gs->width && new_y >= 0 && new_y < gs->height && free_cell(gs->board[new_x + new_y * gs->width]));

}

int main(int argc, char *argv[]) {
    int num_players = 0;
    int delay = DELAY_DEFAULT;
    int seed = time(NULL);
    int timeout = TIMEOUT_DEFAULT;
    char* view = NULL;
    int width = WIDTH_DEFAULT;
    int height = HEIGHT_DEFAULT;

    char* players_bins[MAX_PLAYERS];

    int opt;
    while ((opt = getopt(argc, argv, "w:h:p:d:s:t:v:")) != -1) {
        switch (opt) {
            case 'w':
                width = atoi(optarg);
                break;
            case 'h':
                height = atoi(optarg);
                break;
            case 'p':
                while (optind < argc && (argv[optind][0] == '.' || argv[optind][0] == '/')) {
                    players_bins[num_players++] = argv[optind++];
                }
                break;
            case 'd':
                delay = atoi(optarg);
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 'v':
                view = optarg;
                break;
            default:
                perror("Not valid argument");
                break;
        }
    }

    game_state_t* gs = allocate_game_state_shm(width, height);

    srand(seed);

    gs->width = width;
    gs->height = height;
    gs->num_players = num_players;
    gs->finished = false;

    for (int i = 0; i < height * width; i++) {
        gs->board[i] = ((rand() % MAX_BOARD_VALUE) + MIN_BOARD_VALUE);
    }

    sync_t* sync = allocate_sync_shm();
    init_sync(sync);

    for (int i = 0; i < num_players; i++) {
        gs->players[i].blocked = false;
        gs->players[i].invalids = 0;
        sprintf(gs->players[i].name, "Player %d", i + 1);
        gs->players[i].score = 0;
        gs->players[i].valids = 0;
        set_valid_positions(gs, i);
    }

    int fds[num_players][2];
    char width_s[6]; // ver bien si poner 6 u otra cosa
    char height_s[6];

    for (int i = 0; i < num_players; i++) {

        if (pipe(fds[i]) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t pid_p = fork();

        if (pid_p < 0) {
            perror("fork p");
            return 1;
        }

        if (pid_p == 0) {
            //codigo hijo
            if (dup2(fds[i][1], STDOUT_FILENO) == -1) {
                perror("dup2 p");
                _exit(1);
            }

            close(fds[i][0]);
            close(fds[i][1]);

            sprintf(width_s, "%d", width);
            sprintf(height_s, "%d", height);

            char *argv_c[] = { players_bins[i], width_s, height_s, NULL };
            execve(players_bins[i], argv_c, NULL);
            perror("exec player");
            _exit(1);
        }

        //codigo padre
        gs->players[i].pid = pid_p;
        close(fds[i][1]);
    }

    int i = 0;
    int blocked_players;
    time_t start_time = time(NULL);
    int n;
    char mov[1];
    fd_set read_fds;
    int max_fd = 0;

    for (int j = 0; j < num_players; j++) {
        if (fds[j][0] > max_fd) {
            max_fd = fds[j][0];
        }
    }

    int start_player = 0;

    while (!gs->finished) {
        FD_ZERO(&read_fds);

        for (int j = 0; j < num_players; j++) {
            int player_idx = (start_player + j) % num_players;
            if (!gs->players[player_idx].blocked) {
                FD_SET(fds[player_idx][0], &read_fds);
            }
        }

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            break;
        }

        for (int j = 0; j < num_players; j++) {
            int player_idx = (start_player + j) % num_players;
            if (FD_ISSET(fds[player_idx][0], &read_fds)) {
                n = read(fds[player_idx][0], mov, 1);
                if (n == 0) {
                    gs->players[player_idx].blocked = true;
                } else if (n == 1) {
                    int processed_move = mov[0] - '0';

                    int new_x = gs->players[player_idx].x + DIRS[processed_move][0];
                    int new_y = gs->players[player_idx].y + DIRS[processed_move][1];

                    reader_lock(sync);
                    bool update = is_valid_move(player_idx, new_x, new_y, gs);
                    reader_unlock(sync);

                    writer_lock(sync);
                    if (update) {
                        gs->players[player_idx].x = new_x;
                        gs->players[player_idx].y = new_y;
                        gs->players[player_idx].score += gs->board[new_x + new_y * gs->width];
                        gs->players[player_idx].valids++;
                        gs->board[new_x + new_y * gs->width] = -player_idx;
                        start_time = time(NULL);
                    } else {
                        gs->players[player_idx].invalids++;
                    }
                    writer_unlock(sync);

                    sem_post(&sync->move_signal[player_idx]);
                }
            }
        }

        if (difftime(time(NULL), start_time) > timeout) {
            gs->finished = true;
        }

        blocked_players = 0;

        if (!gs->finished) {
            for (int j = 0; j < num_players; j++) {
                if (gs->players[j].blocked) {
                    blocked_players++;
                }
            }
            if (blocked_players == num_players) {
                gs->finished = true;
            }
        }

        start_player = (start_player + 1) % num_players;
    }

    wait_all(gs, NULL); //hay que poner el PID del view aca

    print_winners(gs);

    close_fds(fds, num_players);
    destroy_sync(sync);
    cleanup_shared_memory();
    return 0;
}
