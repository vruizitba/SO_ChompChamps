#ifndef ARGS_H
#define ARGS_H

#include "common.h"

typedef struct {
    unsigned short width;
    unsigned short height;
    int delay;
    int timeout;
    char *player_paths[MAX_PLAYERS];
    char *view_path;
    unsigned int seed;
} args_t;

/**
 * Parses command-line arguments and fills the args structure.
 * Supported options:
 *  -w <width>    Board width
 *  -h <height>   Board height
 *  -d <delay>    Delay in ms between frames for the view
 *  -t <timeout>  Seconds of inactivity (sin movimientos válidos) to end the game
 *  -s <seed>     RNG seed (if omitted, a default seed is set beforehand)
 *  -v <view>     Path to view executable
 *  -p <player1> [player2 ...]  Player executable paths (1..MAX_PLAYERS)
 * The caller must initialize args with defaults (initialize_default_args) before calling.
 * Player paths appearing after -p (until next option starting with '-') are collected.
 *
 * @param argc Argument count from main
 * @param argv Argument vector from main
 * @param args Pointer to args_t already initialized with defaults; fields are overwritten according to options
 * @return Number of players parsed (>=1) on success, -1 on error (invalid option, missing value, or no players)
 */
int parse_args(int argc, char **argv, args_t *args);

#endif // ARGS_H
