// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "master.h"
#include "sync.h"
#include "util.h"

#define MAX_BOARD_VALUE 9
#define MIN_BOARD_VALUE 1
#define MAX_INT_SIZE 12 // Tamaño 12 = 10 digitos + signo + \0  | Int máximo = 2147483647 (10 digitos)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/select.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

static void print_player_exit(const game_state_t* gs, int idx, int exit_code) {
    char namebuf[sizeof(gs->players[0].name)];
    snprintf(namebuf, sizeof(namebuf), "%s", gs->players[idx].name);
    printf("Player %s exited (%d) with a score of %u / %u / %u\n",
           namebuf,
           exit_code,
           gs->players[idx].score,
           gs->players[idx].valids,
           gs->players[idx].invalids);
    fflush(stdout);
}

static void prepare_fd_set(fd_set* read_fds, const game_state_t* gs, int fds[][2], int num_players, int start_player, sync_t * sync) {
    FD_ZERO(read_fds);
    for (int j = 0; j < num_players; j++) {
        int player_idx = (start_player + j) % num_players;
        reader_lock(sync);
        if (!gs->players[player_idx].blocked) {
            FD_SET(fds[player_idx][0], read_fds);
        }
        reader_unlock(sync);
    }
}

void handle_player_event(int player_idx, game_state_t* gs, sync_t* sync, int player_fd, time_t* last_successful_move_time) {
    unsigned char mov;
    ssize_t n = read(player_fd, &mov, 1);
    if (n == 0) {
        reader_lock(sync);
        gs->players[player_idx].blocked = true;
        reader_unlock(sync);
        return;
    }
    if (n != 1) {
        return;
    }
    if (mov > 7) {
        writer_lock(sync);
        gs->players[player_idx].invalids++;
        writer_unlock(sync);
        sem_post(&sync->move_signal[player_idx]);
        return;
    }

    writer_lock(sync);
    int new_x = gs->players[player_idx].x + DIRS[mov][0];
    int new_y = gs->players[player_idx].y + DIRS[mov][1];
    writer_unlock(sync);

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
        *last_successful_move_time = time(NULL);
    } else {
        gs->players[player_idx].invalids++;
    }
    writer_unlock(sync);

    sem_post(&sync->move_signal[player_idx]);
}

void check_timeout_and_finish(game_state_t* gs, sync_t* sync, const args_t* args, time_t last_successful_move_time) {
    reader_lock(sync);
    bool is_finished = gs->finished;
    reader_unlock(sync);

    if (is_finished) {
        return;
    }

    if (difftime(time(NULL), last_successful_move_time) > args->timeout) {
        writer_lock(sync);
        gs->finished = true;
        writer_unlock(sync);
    }
}

void check_all_blocked_and_finish(game_state_t* gs, sync_t* sync) {
    reader_lock(sync);
    bool is_finished = gs->finished;
    reader_unlock(sync);

    if (is_finished) {
        return;
    }

    int blocked = 0;
    reader_lock(sync);
    unsigned int num_players = gs->num_players;
    for (unsigned i = 0; i < num_players; i++) {
        if (gs->players[i].blocked) {
            blocked++;
        }
    }
    reader_unlock(sync);

    if (blocked == (int)num_players) {
        writer_lock(sync);
        gs->finished = true;
        writer_unlock(sync);
    }
}

static void update_view(game_state_t* gs, sync_t* sync, const args_t* args) {
    if (args->view_path == NULL) {
        return;
    }
    sem_post(&sync->drawing_signal);
    sem_wait(&sync->not_drawing_signal);
    reader_lock(sync);
    if (!gs->finished) {
        struct timespec ts = { .tv_sec = args->delay / 1000, .tv_nsec = (args->delay % 1000) * 1000000L };
        nanosleep(&ts, NULL);
    }
    reader_unlock(sync);
}

void print_winners(game_state_t* gs) {
    unsigned int best_score = 0;
    int best_players[MAX_PLAYERS], count = 0;

    for (unsigned int i = 0; i < gs->num_players; i++) {
        if (gs->players[i].score > best_score) {
            best_score = gs->players[i].score;
            count = 0;
            best_players[count++] = i;
        } else if (gs->players[i].score == best_score) {
            best_players[count++] = i;
        }
    }

    if (count == 1) {
        printf("Winner: %s (Score: %u)\n", gs->players[best_players[0]].name, gs->players[best_players[0]].score);
        return;
    }

    unsigned int min_valids = UINT_MAX;
    for (int i = 0; i < count; i++) {
        if (gs->players[best_players[i]].valids < min_valids)
            min_valids = gs->players[best_players[i]].valids;
    }

    int tied_by_valids[MAX_PLAYERS], tied_count = 0;
    for (int i = 0; i < count; i++) {
        if (gs->players[best_players[i]].valids == min_valids)
            tied_by_valids[tied_count++] = best_players[i];
    }

    if (tied_count == 1) {
        printf("Winner: %s (Score: %u, Valids: %u)\n",
               gs->players[tied_by_valids[0]].name,
               gs->players[tied_by_valids[0]].score,
               gs->players[tied_by_valids[0]].valids);
        return;
    }

    unsigned int min_invalids = UINT_MAX;
    int final_winners[MAX_PLAYERS], final_count = 0;

    for (int i = 0; i < tied_count; i++) {
        int player_idx = tied_by_valids[i];
        unsigned int player_invalids = gs->players[player_idx].invalids;
        if (player_invalids < min_invalids) {
            min_invalids = player_invalids;
            final_count = 0;
            final_winners[final_count++] = player_idx;
        } else if (player_invalids == min_invalids) {
            final_winners[final_count++] = player_idx;
        }
    }

    if (final_count == 1) {
        printf("Winner: %s (Score: %u, Valids: %u, Invalids: %u)\n",
               gs->players[final_winners[0]].name,
               gs->players[final_winners[0]].score,
               gs->players[final_winners[0]].valids,
               gs->players[final_winners[0]].invalids);
        return;
    }

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

void wait_all(game_state_t* gs, pid_t view) {
    int remaining = (int)gs->num_players + ((view != -1) ? 1 : 0);
    int status;

    bool hold_player_prints = (view != -1);
    int buf_idx[MAX_PLAYERS];
    int buf_exit[MAX_PLAYERS];
    int buf_count = 0;

    while (remaining > 0) {
        pid_t pid = wait(&status);
        if (pid == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        remaining--;

        int exit_code = 0;
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exit_code = 128 + WTERMSIG(status);
        }

        if (view != -1 && pid == view) {
            printf("View exited (%d)\n", exit_code);
            fflush(stdout);
            hold_player_prints = false;
            for (int b = 0; b < buf_count; ++b) {
                int idx = buf_idx[b];
                print_player_exit(gs, idx, buf_exit[b]);
            }
            buf_count = 0;
            continue;
        }

        int idx = -1;
        bool found = false;
        for (unsigned int i = 0; i < gs->num_players && !found; i++) {
            if (gs->players[i].pid == pid) {
                idx = (int)i;
                found = true;
            }
        }

        if (idx >= 0) {
            if (hold_player_prints) {
                if (buf_count < MAX_PLAYERS) {
                    buf_idx[buf_count] = idx;
                    buf_exit[buf_count] = exit_code;
                    buf_count++;
                } else {
                    print_player_exit(gs, idx, exit_code);
                }
            } else {
                print_player_exit(gs, idx, exit_code);
            }
        }
    }
}

void set_valid_positions(game_state_t* gs, int player_pos) {
    int x, y;
    do {
        x = rand() % gs->width;
        y = rand() % gs->height;
    } while (!is_free_cell(gs->board[x + y * gs->width]));
    gs->players[player_pos].x = x;
    gs->players[player_pos].y = y;
    gs->board[x + y * gs->width] = -player_pos;
}

void close_fds(int fds[][2], int num_players) {
    for (int i = 0; i < num_players; i++) {
        close(fds[i][0]);
    }
}

void initialize_default_args(args_t* args) {
    args->width = WIDTH_DEFAULT;
    args->height = HEIGHT_DEFAULT;
    args->delay = DELAY_DEFAULT;
    args->timeout = TIMEOUT_DEFAULT;
    args->seed = (unsigned int)time(NULL);
    args->view_path = NULL;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        args->player_paths[i] = NULL;
    }
}

void init_game_state(game_state_t* gs, args_t* args, int num_players) {
    gs->width = args->width;
    gs->height = args->height;
    gs->num_players = num_players;
    gs->finished = false;

    srand(args->seed);
    for (int i = 0; i < args->height * args->width; i++) {
        gs->board[i] = ((rand() % MAX_BOARD_VALUE) + MIN_BOARD_VALUE);
    }

    for (int i = 0; i < num_players; i++) {
        gs->players[i].blocked = false;
        gs->players[i].invalids = 0;
        
        const char* full_path = args->player_paths[i];
        const char* executable_name = strrchr(full_path, '/');
        if (executable_name != NULL) {
            executable_name++;
        } else {
            executable_name = full_path;
        }
        snprintf(gs->players[i].name, sizeof(gs->players[i].name), "%s - P%d", executable_name, i + 1);
        
        gs->players[i].score = 0;
        gs->players[i].valids = 0;
        set_valid_positions(gs, i);
    }
}

pid_t create_view_process(const char* view_path, const char* width_s, const char* height_s) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork v");
        return -1;
    }

    if (pid == 0) {
        char *argv[] = { (char*)view_path, (char*)width_s, (char*)height_s, NULL };
        execve(view_path, argv, NULL);
        perror("exec view");
        _exit(1);
    }

    return pid;
}

pid_t create_player_process(const char* player_path, const char* width_s, const char* height_s, int pipe_fd[2]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork player");
        return -1;
    }
    if (pid == 0) {
        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
            perror("dup2 player");
            _exit(1);
        }
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        char *argv[] = { (char*)player_path, (char*)width_s, (char*)height_s, NULL };
        execve(player_path, argv, NULL);
        perror("exec player");
        _exit(1);
    }
    close(pipe_fd[1]);
    return pid;
}

void start_view (sync_t* sync) {
    sem_post(&sync->drawing_signal);
    sem_wait(&sync->not_drawing_signal);
}

void play(game_state_t* gs, sync_t* sync, const args_t* args, int fds[][2], int num_players, int max_fd) {
    time_t last_successful_move_time = time(NULL);
    fd_set read_fds;
    bool is_player_blocked;
    int start_player = 0;

    if (args->view_path != NULL) {
        start_view(sync);
    }

    do {
        prepare_fd_set(&read_fds, gs, fds, num_players, start_player, sync);

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            break;
        }

        for (int j = 0; j < num_players; j++) {
            int player_idx = (start_player + j) % num_players;
            reader_lock(sync);
            is_player_blocked = gs->players[player_idx].blocked;
            reader_unlock(sync);

            if (!is_player_blocked && FD_ISSET(fds[player_idx][0], &read_fds)) {
                handle_player_event(player_idx, gs, sync, fds[player_idx][0], &last_successful_move_time);
            }
        }

        check_timeout_and_finish(gs, sync, args, last_successful_move_time);
        check_all_blocked_and_finish(gs, sync);
        update_view(gs, sync, args);

        start_player = (start_player + 1) % num_players;
    } while (!gs->finished);
}

int main(int argc, char *argv[]) {
    args_t args;
    initialize_default_args(&args);

    int num_players = parse_args(argc, argv, &args);
    if (num_players < 0) {
        exit(1);
    }
    if (num_players == 0) {
        fprintf(stderr, "No players provided. Exiting.\n");
        exit(1);
    }

    game_state_t* gs = allocate_game_state_shm(args.width, args.height);
    if (gs == NULL) {
        fprintf(stderr, "Failed to allocate game state shared memory\n");
        exit(1);
    }
    init_game_state(gs, &args, num_players);

    sync_t* sync = allocate_sync_shm();
    if (sync == NULL) {
        fprintf(stderr, "Failed to allocate sync shared memory\n");
        exit(1);
    }
    init_sync(sync);

    char width_s[MAX_INT_SIZE];
    char height_s[MAX_INT_SIZE];
    snprintf(width_s, sizeof(width_s), "%d", args.width);
    snprintf(height_s, sizeof(height_s), "%d", args.height);

    pid_t pid_v = -1;
    if (args.view_path != NULL) {
        pid_v = create_view_process(args.view_path, width_s, height_s);
    }

    int fds[MAX_PLAYERS][2];
    int max_fd = -1;
    for (int i = 0; i < num_players; i++) {
        if (pipe(fds[i]) == -1) {
            perror("pipe");
            exit(1);
        }
        pid_t pid_p = create_player_process(args.player_paths[i], width_s, height_s, fds[i]);
        if (pid_p < 0) {
            fprintf(stderr, "Failed to create player process %d\n", i);
            exit(1);
        }
        gs->players[i].pid = pid_p;
        if (fds[i][0] > max_fd) {
            max_fd = fds[i][0];
        }
    }

    play(gs, sync, &args, fds, num_players, max_fd);

    wait_all(gs, pid_v);
    print_winners(gs);

    close_fds(fds, num_players);
    destroy_sync(sync);
    cleanup_shared_memory();
    return 0;
}
