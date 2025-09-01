#ifndef ARGS_H
#define ARGS_H

#include "common.h"
#include <time.h>
#include <stdlib.h>

typedef struct {
    unsigned short width;
    unsigned short height;
    int delay;
    int timeout;
    char *player_paths[MAX_PLAYERS];
    char *view_path;
    unsigned int seed;
} args_t;

int parse_args(int argc, char **argv, args_t *args);

#endif // ARGS_H