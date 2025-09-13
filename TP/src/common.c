// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "common.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

game_state_t* allocate_game_state_shm(unsigned short width, unsigned short height) {
    size_t board_size = width * height * sizeof(int);
    size_t total_size = sizeof(game_state_t) + board_size;
    
    int shm_fd = shm_open(SHM_STATE, O_CREAT | O_RDWR, 0777);
    if (shm_fd == -1) {
        perror("shm_open failed for game state");
        return NULL;
    }
    
    if (ftruncate(shm_fd, total_size) == -1) {
        perror("ftruncate failed for game state");
        shm_unlink(SHM_STATE);
        return NULL;
    }
    
    game_state_t* game_state = (game_state_t*)mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap failed for game state");
        shm_unlink(SHM_STATE);
        return NULL;
    }
    
    memset(game_state, 0, total_size);

    close(shm_fd);
    
    return game_state;
}

sync_t* allocate_sync_shm(void) {
    size_t size = sizeof(sync_t);
    
    int shm_fd = shm_open(SHM_SYNC, O_CREAT | O_RDWR, 0777);
    if (shm_fd == -1) {
        perror("shm_open failed for sync");
        return NULL;
    }
    
    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate failed for sync");
        shm_unlink(SHM_SYNC);
        return NULL;
    }
    
    sync_t* sync = (sync_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap failed for sync");
        shm_unlink(SHM_SYNC);
        return NULL;
    }
    
    sync->reader_count = 0;

    close(shm_fd);
    
    return sync;
}

game_state_t* attach_game_state_shm_readonly(void) {
    int shm_fd = shm_open(SHM_STATE, O_RDONLY, 0);
    if (shm_fd == -1) {
        perror("shm_open failed for game state attachment");
        return NULL;
    }
    
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("fstat failed for game state");
        return NULL;
    }
    
    game_state_t* game_state = (game_state_t*)mmap(NULL, shm_stat.st_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap failed for game state attachment");
        return NULL;
    }

    close(shm_fd);
    
    return game_state;
}

sync_t* attach_sync_shm(void) {
    size_t size = sizeof(sync_t);
    
    int shm_fd = shm_open(SHM_SYNC, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("shm_open failed for sync attachment");
        return NULL;
    }
    
    sync_t* sync = (sync_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sync == MAP_FAILED) {
        perror("mmap failed for sync attachment");
        return NULL;
    }

    close(shm_fd);
    
    return sync;
}

void cleanup_shared_memory(void) {
    if (shm_unlink(SHM_STATE) == -1) {
        perror("shm_unlink failed for game state");
    }
    
    if (shm_unlink(SHM_SYNC) == -1) {
        perror("shm_unlink failed for sync");
    }
}
