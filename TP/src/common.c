#include "common.h"
#include "sync.h"
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
    game_state_t* game_state = (game_state_t*)mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap failed for game state");
        shm_unlink(SHM_STATE);
        return NULL;
    }
    
    memset(game_state, 0, total_size);
    
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
    sync_t* sync = (sync_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap failed for sync");
        shm_unlink(SHM_SYNC);
        return NULL;
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
    game_state_t* game_state = (game_state_t*)mmap(NULL, shm_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
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
    sync_t* sync = (sync_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
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
