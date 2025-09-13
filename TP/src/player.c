// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "common.h"
#include "util.h"
#include "ai.h"
#include "player.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>

#include "sync.h"

int main(int argc, char *argv[]) {
    if(argc < 3){
        perror("Invalid player arguments");
        exit(1);
    }

    unsigned short width = (unsigned short)atoi(argv[1]);
    unsigned short height = (unsigned short)atoi(argv[2]);

    game_state_t *game_state = attach_game_state_shm_readonly();
    if(game_state == NULL) {
        perror("Failed to attach game state shared memory to player");
        exit(1);
    }

    sync_t *sync = attach_sync_shm();
    if(sync == NULL) {
        perror("Failed to attach sync shared memory to player");
        exit(1);
    }

    reader_lock(sync);
    if (game_state->width != width || game_state->height != height) {
        fprintf(stderr, "Player: dimension mismatch (argv %ux%u vs shm %ux%u)\n",
                width, height, game_state->width, game_state->height);
        exit(1);
    }
    reader_unlock(sync);

    int id = find_player_id(game_state, getpid(), sync);
    if(id < 0) {
        perror("Failed to find player id");
        exit(1);
    }

    int move_dir[2] = {0, 0};

    while(sem_wait(&sync->move_signal[id]) || choose_best_move(move_dir, game_state, sync, id) != -1){
        unsigned char dir_to_send = direction_to_char(move_dir);
        if(dir_to_send == 255) {
            perror("Invalid move direction");
            continue;
        }
        printf("%c", dir_to_send);
        fflush(stdout);
    }
    fclose(stdout);

    return 0;
}

int find_player_id(const game_state_t *gs, pid_t pid, sync_t * sync) {
    pid_t player_pid;
    for (unsigned int i = 0; i < MAX_PLAYERS; i++) {
        reader_lock(sync);
        player_pid = gs->players[i].pid;
        reader_unlock(sync);
        if (player_pid == pid) {
            return i;
        }
    }
    return -1;
}
