#include "common.h"
#include "sync.h"

void init_sync(sync_t *s) {
    sem_init(&s->drawing_signal, 1, 0);
    sem_init(&s->not_drawing_signal, 1, 0);
    sem_init(&s->accessor_queue_signal, 1, 1);
    sem_init(&s->full_access_signal, 1, 1);
    sem_init(&s->reader_count_protect_signal, 1, 1);
    s->reader_count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        sem_init(&s->move_signal[i], 1, 0);
    }
}

void destroy_sync(sync_t *s) {
    sem_destroy(&s->drawing_signal);
    sem_destroy(&s->not_drawing_signal);
    sem_destroy(&s->accessor_queue_signal);
    sem_destroy(&s->full_access_signal);
    sem_destroy(&s->reader_count_protect_signal);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        sem_destroy(&s->move_signal[i]);
    }
}

void reader_lock(sync_t *s) {
    sem_wait(&s->accessor_queue_signal);

    sem_wait(&s->reader_count_protect_signal);
    if (s->reader_count == 0) {
        sem_wait(&s->full_access_signal);
    }
    s->reader_count++;
    sem_post(&s->reader_count_protect_signal);

    sem_post(&s->accessor_queue_signal);
}

void reader_unlock(sync_t *s) {
    sem_wait(&s->reader_count_protect_signal);
    s->reader_count--;
    if (s->reader_count == 0) {
        sem_post(&s->full_access_signal);
    }
    sem_post(&s->reader_count_protect_signal);
}
