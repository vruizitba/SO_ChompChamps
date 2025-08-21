//
// Created by Valentin Ruiz on 20/08/2025.
//

#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_STATE "/game_state"
#define SHM_SYNC  "/game_sync"

#define MAX_PLAYERS 9

typedef struct {
    char name[16];
    unsigned int score;
    unsigned int invalids;
    unsigned int valids;
    unsigned short x, y;
    pid_t pid;
    bool blocked;
} player_t;

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned int num_players;
    player_t players[MAX_PLAYERS];
    bool finished;
    int board[];
} game_state_t;

typedef struct {
    sem_t drawing_signal;
    sem_t not_drawing_signal;
    sem_t accessor_queue_signal;
    sem_t full_access_signal;
    sem_t reader_count_protect_signal;
    unsigned int reader_count;
    sem_t move_signal[MAX_PLAYERS];
} sync_t;

// Shared memory utility functions
game_state_t* allocate_game_state_shm(unsigned short width, unsigned short height);
sync_t* allocate_sync_shm(void);
game_state_t* attach_game_state_shm(void);
sync_t* attach_sync_shm(void);
void cleanup_shared_memory(void);

#endif //COMMON_H