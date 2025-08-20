//
// Created by Valentin Ruiz on 20/08/2025.
//

#ifndef COMMONS_H
#define COMMONS_H

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
    sem_t drawing_sem;
    sem_t not_drawing_sem;
    sem_t C;
    sem_t D;
    sem_t E;
    unsigned int F;
    sem_t G[MAX_PLAYERS];
} Sync;

#endif //COMMONS_H
