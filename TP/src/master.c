//
// Created by Valentin Ruiz on 27/08/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/select.h>
#include <limits.h>

#include "master.h"
#include "sync.h"
#include "common.h"
#include "util.h"
#include "args.h"

// Helper: build fd_set for current turn ordering
static void prepare_fd_set(fd_set* read_fds, const game_state_t* gs, int fds[][2], int num_players, int start_player) {
    FD_ZERO(read_fds);
    for (int j = 0; j < num_players; j++) {
        int player_idx = (start_player + j) % num_players;
        if (!gs->players[player_idx].blocked) {
            FD_SET(fds[player_idx][0], read_fds);
        }
    }
}

// Helper: process a single player's pending move if ready
static void handle_player_event(int player_idx, game_state_t* gs, sync_t* sync, int player_fd, time_t* last_successful_move_time) {
    unsigned char mov;
    ssize_t n = read(player_fd, &mov, 1);
    if (n == 0) { // EOF -> player done
        gs->players[player_idx].blocked = true;
        return;
    } else if (n != 1) {
        return; // ignore partial/erroneous reads
    }
    int processed_move = mov; // 0..7 expected (players control this)
    if (processed_move < 0 || processed_move > 7) {
        gs->players[player_idx].invalids++;
        sem_post(&sync->move_signal[player_idx]);
        return;
    }
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
        *last_successful_move_time = time(NULL);
    } else {
        gs->players[player_idx].invalids++;
    }
    writer_unlock(sync);

    sem_post(&sync->move_signal[player_idx]);
}

// Helper: timeout end condition
static void check_timeout_and_finish(game_state_t* gs, sync_t* sync, const args_t* args, time_t last_successful_move_time) {
    if (gs->finished) return;
    if (difftime(time(NULL), last_successful_move_time) > args->timeout) {
        writer_lock(sync);
        if (!gs->finished) gs->finished = true;
        writer_unlock(sync);
    }
}

// Helper: all players blocked end condition
static void check_all_blocked_and_finish(game_state_t* gs, sync_t* sync) {
    if (gs->finished) return;
    int blocked = 0;
    for (unsigned i = 0; i < gs->num_players; i++) {
        if (gs->players[i].blocked) blocked++;
    }
    if (blocked == (int)gs->num_players) {
        writer_lock(sync);
        if (!gs->finished) gs->finished = true;
        writer_unlock(sync);
    }
}

// Helper: update view and optionally delay
static void update_view(game_state_t* gs, sync_t* sync, const args_t* args) {
    if (args->view_path == NULL) return;
    sem_post(&sync->drawing_signal);
    sem_wait(&sync->not_drawing_signal);
    if (!gs->finished) {
        struct timespec ts = { .tv_sec = args->delay / 1000, .tv_nsec = (args->delay % 1000) * 1000000L };
        nanosleep(&ts, NULL);
    }
}

void print_winners(game_state_t* gs) {
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
    int min_valids = INT_MAX;
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
    int min_invalids = INT_MAX;
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

void wait_all(game_state_t* gs, pid_t view) {
    int status;
    for (int i = 0; i < gs->num_players; i++) {
        waitpid(gs->players[i].pid, &status, 0);
    }

    if (view != -1) {
        waitpid(view, &status, 0);
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
        snprintf(gs->players[i].name, sizeof(gs->players[i].name), "Player %d", i + 1);
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
        // Child process
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
    // Parent process
    close(pipe_fd[1]);
    return pid;
}

void start_view (sync_t* sync) {
    sem_post(&sync->drawing_signal);
    sem_wait(&sync->not_drawing_signal);
}

// New function: encapsulates the gameplay loop
void play(game_state_t* gs, sync_t* sync, const args_t* args, int fds[][2], int num_players, int max_fd) {
    unsigned int blocked_players = 0; // retained (not used directly now but kept for minimal diff potential)
    time_t last_successful_move_time = time(NULL);
    fd_set read_fds;
    int start_player = 0;

    if (args->view_path != NULL) {
        start_view(sync);
    }

    do {
        prepare_fd_set(&read_fds, gs, fds, num_players, start_player);

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            break;
        }

        for (int j = 0; j < num_players; j++) {
            int player_idx = (start_player + j) % num_players;
            if (!gs->players[player_idx].blocked && FD_ISSET(fds[player_idx][0], &read_fds)) {
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
