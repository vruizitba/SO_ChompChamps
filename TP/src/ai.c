#include "player.h"
#include "common.h"
#include "util.h"
#include "sync.h"
#include <stdio.h>

static inline int in_bounds(const game_state_t *gs, int x, int y) {
    return x >= 0 && y >= 0 && x < (int)gs->width && y < (int)gs->height;
}
static inline int cell(const game_state_t *gs, int x, int y) {
    return gs->board[y * gs->width + x];
}
static inline int is_free_cell(int v) {
    return v >= 1 && v <= 9;
}

int choose_best_move(int *move, const game_state_t *gs, sync_t *sync, int id) {
    reader_lock(sync);
    int x = gs->players[id].x;
    int y = gs->players[id].y;

    for (int k = 0; k < 8; ++k) {
        int nx = x + DIRS[k][0];
        int ny = y + DIRS[k][1];
        if (in_bounds(gs, nx, ny) && is_free_cell(cell(gs, nx, ny))){
            move[0] = DIRS[k][0];
            move[1] = DIRS[k][1];
            reader_unlock(sync);
            return 0;
        }
    }
    reader_unlock(sync);
    return -1; // No valid moves found
}
