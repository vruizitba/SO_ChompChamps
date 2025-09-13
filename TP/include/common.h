// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <sys/types.h>
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

/**
 * Allocate and map a shared memory segment for a game_state_t plus its board.
 * Size = sizeof(game_state_t) + (width * height * sizeof(int)). Memory is zeroed.
 * On failure prints an error (perror) and returns NULL.
 * Caller must later (once globally) call cleanup_shared_memory() to unlink the name; munmap is implicit on process exit.
 * @param width  Board width (>0)
 * @param height Board height (>0)
 * @return Pointer to writable shared game_state_t or NULL on error.
 */
 game_state_t* allocate_game_state_shm(unsigned short width, unsigned short height);

/**
 * Allocate and map a shared memory segment for synchronization primitives (sync_t).
 * Initializes reader_count to 0; semaphores are not initialized here (caller should call init_sync()).
 * On failure prints an error and returns NULL.
 * @return Pointer to writable shared sync_t or NULL on error.
 */
 sync_t* allocate_sync_shm(void);

/**
 * Attach (read-only) to an existing game state shared memory object created by allocate_game_state_shm.
 * Mapping is PROT_READ; writing through this pointer is undefined behavior.
 * On failure prints an error and returns NULL.
 * @return Pointer to read-only mapped game_state_t or NULL on error.
 */
 game_state_t* attach_game_state_shm_readonly(void);

/**
 * Attach (read/write) to an existing sync shared memory object created by allocate_sync_shm.
 * On failure prints an error and returns NULL.
 * @return Pointer to mapped sync_t or NULL on error.
 */
 sync_t* attach_sync_shm(void);

/**
 * Unlink (remove) the named shared memory objects (game state and sync).
 * Safe to call multiple times; errors are printed but ignored.
 * Does NOT unmap existing mappings; each process may still hold its own mapping until exit.
 */
 void cleanup_shared_memory(void);

#endif //COMMON_H
