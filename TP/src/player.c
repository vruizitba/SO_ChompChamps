#include "common.h"
#include "sync.h"
#include "util.h"
#include "ai.h"
#include "player.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if(argc < 3){
        perror("Invalid player arguments");
        exit(1);
    }
    unsigned short width = atoi(argv[1]);
    unsigned short height = atoi(argv[2]);

    game_state_t *game_state = attach_game_state_shm();
    if(game_state == MAP_FAILED) {
        perror("Failed to attach game state shared memory");
        exit(1);
    }

    sync_t *sync = attach_sync_shm();
    if(sync == MAP_FAILED) {
        perror("Failed to attach sync shared memory");
        exit(1);
    }

    int id = find_player_id(game_state, getpid());
    if(id < 0){
        perror("Failed to find player id");
        exit(1);
    }

    int move_dir[2] = {0, 0};

    while(choose_best_move(move_dir, game_state, sync, id) != -1){
        sem_wait(&sync->move_signal[id]);
        unsigned char dir_to_send = direction_to_char(move_dir);
        if(dir_to_send == 255) {
            perror("Invalid move direction");
            continue;
        }
        //by this point it is connected by pipe to the master
        printf("%c", dir_to_send);
        fflush(stdout);
    }

    // When the loop ends, it means there are no more moves.
    // Send EOF to the master by closing stdout.
    fclose(stdout);

    return 0;
}

int find_player_id(const game_state_t *gs, pid_t pid) {
    for (unsigned int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].pid == pid) {
            return i;
        }
    }
    return -1; // Player not found
}