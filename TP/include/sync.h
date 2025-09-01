#ifndef SYNC_H
#define SYNC_H

#include "common.h"

void init_sync(sync_t *s);

void destroy_sync(sync_t *s);

void reader_lock(sync_t *s);
void reader_unlock(sync_t *s);

void writer_lock(sync_t *s);
void writer_unlock(sync_t *s);

#endif //SYNC_H