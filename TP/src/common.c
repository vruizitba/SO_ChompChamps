//
// Created by Valentin Ruiz on 20/08/2025.
//

#include "../include/common.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

game_state_t* allocate_game_state_shm(unsigned short width, unsigned short height) {
    // Calculate the size needed for the game state including the board
    size_t board_size = width * height * sizeof(int);
    size_t total_size = sizeof(game_state_t) + board_size;
    
    // Create shared memory object
    int shm_fd = shm_open(SHM_STATE, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed for game state");
        return NULL;
    }
    
    // Set the size of the shared memory object
    if (ftruncate(shm_fd, total_size) == -1) {
        perror("ftruncate failed for game state");
        shm_unlink(SHM_STATE);
        return NULL;
    }
    
    // Map the shared memory object
    game_state_t* game_state = (game_state_t*)mmap(NULL, total_size, 
                                                   PROT_READ | PROT_WRITE, 
                                                   MAP_SHARED, shm_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap failed for game state");
        shm_unlink(SHM_STATE);
        return NULL;
    }
    
    // Initialize the game state
    memset(game_state, 0, total_size);
    game_state->width = width;
    game_state->height = height;
    game_state->num_players = 0;
    game_state->finished = false;
    
    // Initialize the board with 1s (chocolate pieces)
    for (int i = 0; i < width * height; i++) {
        game_state->board[i] = 1;
    }
    
    // The top-left corner (0,0) should be poison
    game_state->board[0] = 0;
    
    return game_state;
}

sync_t* allocate_sync_shm(void) {
    size_t size = sizeof(sync_t);
    
    // Create shared memory object
    int shm_fd = shm_open(SHM_SYNC, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed for sync");
        return NULL;
    }
    
    // Set the size of the shared memory object
    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate failed for sync");
        shm_unlink(SHM_SYNC);
        return NULL;
    }
    
    // Map the shared memory object
    sync_t* sync = (sync_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, 
                                 MAP_SHARED, shm_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap failed for sync");
        shm_unlink(SHM_SYNC);
        return NULL;
    }
    
    // Initialize semaphores
    if (sem_init(&sync->drawing_signal, 1, 0) == -1 ||
        sem_init(&sync->not_drawing_signal, 1, 1) == -1 ||
        sem_init(&sync->accessor_queue_signal, 1, 1) == -1 ||
        sem_init(&sync->full_access_signal, 1, 1) == -1 ||
        sem_init(&sync->reader_count_protect_signal, 1, 1) == -1) {
        perror("sem_init failed for sync semaphores");
        munmap(sync, size);
        shm_unlink(SHM_SYNC);
        return NULL;
    }
    
    // Initialize move signals for each player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (sem_init(&sync->move_signal[i], 1, 0) == -1) {
            perror("sem_init failed for move signal");
            // Cleanup already initialized semaphores
            for (int j = 0; j < i; j++) {
                sem_destroy(&sync->move_signal[j]);
            }
            sem_destroy(&sync->drawing_signal);
            sem_destroy(&sync->not_drawing_signal);
            sem_destroy(&sync->accessor_queue_signal);
            sem_destroy(&sync->full_access_signal);
            sem_destroy(&sync->reader_count_protect_signal);
            munmap(sync, size);
            shm_unlink(SHM_SYNC);
            return NULL;
        }
    }
    
    sync->reader_count = 0;
    
    return sync;
}

game_state_t* attach_game_state_shm(void) {
    // Open existing shared memory object
    int shm_fd = shm_open(SHM_STATE, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed for game state attachment");
        return NULL;
    }
    
    // Get the size of the shared memory object
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("fstat failed for game state");
        return NULL;
    }
    
    // Map the shared memory object
    game_state_t* game_state = (game_state_t*)mmap(NULL, shm_stat.st_size, 
                                                   PROT_READ | PROT_WRITE, 
                                                   MAP_SHARED, shm_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap failed for game state attachment");
        return NULL;
    }
    
    return game_state;
}

sync_t* attach_sync_shm(void) {
    size_t size = sizeof(sync_t);
    
    // Open existing shared memory object
    int shm_fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed for sync attachment");
        return NULL;
    }
    
    // Map the shared memory object
    sync_t* sync = (sync_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, 
                                 MAP_SHARED, shm_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap failed for sync attachment");
        return NULL;
    }
    
    return sync;
}

void cleanup_shared_memory(void) {
    // Unlink shared memory objects
    if (shm_unlink(SHM_STATE) == -1) {
        perror("shm_unlink failed for game state");
    }
    
    if (shm_unlink(SHM_SYNC) == -1) {
        perror("shm_unlink failed for sync");
    }
}
