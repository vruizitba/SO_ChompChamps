#ifndef SYNC_H
#define SYNC_H

#include "common.h"

/**
 * Initialize all semaphores in a shared sync_t block.
 * Preconditions: s points to zeroed shared memory from allocate_sync_shm().
 * drawing_signal / not_drawing_signal: handshake master <-> view.
 * accessor_queue_signal: fair queue to serialize access intent.
 * full_access_signal: writer exclusion / blocks while readers present.
 * reader_count_protect_signal: protects reader_count.
 * move_signal[i]: per‑player permit to produce next move.
 * Post: semaphores ready; reader_count = 0.
 * @param s Pointer to shared sync_t structure.
 */
void init_sync(sync_t *s);

/**
 * Destroy (sem_destroy) all semaphores in sync_t. Does not unmap/unlink shared memory.
 * Preconditions: previously initialized with init_sync().
 * @param s Pointer to shared sync_t structure.
 */
void destroy_sync(sync_t *s);

/**
 * Acquire read lock. Multiple readers allowed; first reader acquires full_access_signal.
 * Fairness ensured via accessor_queue_signal.
 * @param s Pointer to shared sync_t structure.
 */
void reader_lock(sync_t *s);

/**
 * Release read lock. Last reader releases full_access_signal.
 * @param s Pointer to shared sync_t structure.
 */
void reader_unlock(sync_t *s);

/**
 * Acquire write lock: take accessor_queue_signal then full_access_signal for exclusive access.
 * @param s Pointer to shared sync_t structure.
 */
void writer_lock(sync_t *s);

/**
 * Release write lock in reverse order, allowing next waiter (reader or writer).
 * @param s Pointer to shared sync_t structure.
 */
void writer_unlock(sync_t *s);

#endif //SYNC_H
