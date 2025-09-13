// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "common.h"
#include "util.h"
#include "sync.h"
#include <stdlib.h>
#include <string.h>

static int min_chebyshev_to_opponent(const game_state_t *gs, int me, int x, int y) {
    int best = 999999;
    for (unsigned int i = 0; i < gs->num_players; ++i) {
        if ((int)i == me) continue;
        int ox = gs->players[i].x;
        int oy = gs->players[i].y;
        int dx = abs(ox - x);
        int dy = abs(oy - y);
        int d  = (dx > dy) ? dx : dy;
        if (d < best) best = d;
    }
    return (best == 999999) ? 99 : best;
}

typedef struct { int x, y, d; } qnode_t;

static int hash_idx(int x, int y, int mask) {
    unsigned int hx = (unsigned int)x * 73856093u;
    unsigned int hy = (unsigned int)y * 19349663u;
    return (int)((hx ^ hy) & (unsigned int)mask);
}

static int was_visited_put(int *vx, int *vy, unsigned char *used, int ht_mask, int x, int y) {
    int h = hash_idx(x, y, ht_mask);
    for (int t = 0; t <= ht_mask; ++t) {
        int i = (h + t) & ht_mask;
        if (!used[i]) {
            used[i] = 1; vx[i] = x; vy[i] = y; return 0;
        }
        if (vx[i] == x && vy[i] == y) {
            return 1;
        }
    }
    return 1;
}

static int territory_potential(const game_state_t *gs, int sx, int sy, int max_depth, int max_nodes, int *total_value) {
    if (!in_bounds(gs, sx, sy) || !is_free_cell(get_cell(gs, sx, sy))) {
        if (total_value) *total_value = 0;
        return 0;
    }

    enum { QMAX = 1024, HT = 2048 };
    qnode_t q[QMAX];
    int head = 0, tail = 0, size = 0;

    int vx[HT], vy[HT]; unsigned char used[HT];
    memset(used, 0, sizeof(used));

    (void)was_visited_put(vx, vy, used, HT - 1, sx, sy);
    q[tail] = (qnode_t){ sx, sy, 0 }; tail = (tail + 1) % QMAX; size++;

    int visited = 0, expanded = 0;
    int value_sum = 0;

    while (size > 0 && expanded < max_nodes) {
        qnode_t cur = q[head]; head = (head + 1) % QMAX; size--;
        visited++;
        
        int cell_value = get_cell(gs, cur.x, cur.y);
        if (is_free_cell(cell_value)) {
            value_sum += cell_value;
        }

        if (cur.d >= max_depth) continue;

        for (int k = 0; k < 8; ++k) {
            int nx = cur.x + DIRS[k][0];
            int ny = cur.y + DIRS[k][1];
            if (!in_bounds(gs, nx, ny)) continue;
            if (!is_free_cell(get_cell(gs, nx, ny))) continue;
            if (was_visited_put(vx, vy, used, HT - 1, nx, ny)) continue;
            if (size < QMAX - 1) {
                q[tail] = (qnode_t){ nx, ny, cur.d + 1 };
                tail = (tail + 1) % QMAX; size++;
            }
            expanded++;
            if (expanded >= max_nodes) break;
        }
    }
    
    if (total_value) *total_value = value_sum;
    return visited;
}

int choose_best_move(int *move, const game_state_t *gs, sync_t *sync, int id) {
    // Normalized weights that sum to 1.0
    const float W_REWARD        = 0.15f;  // Immediate cell value
    const float W_TERRITORY     = 0.25f;  // Territory potential 
    const float W_TERRITORY_VAL = 0.45f;  // Territory value
    const float W_NEAR_OPP      = 0.15f;  // Opponent proximity penalty
    
    // Calculate normalization constants based on actual board size
    const float MAX_CELL_VALUE = 9.0f;
    const float MAX_TERRITORY_NODES = (float)(gs->width * gs->height); // Entire board
    const float MAX_TERRITORY_VALUE = MAX_TERRITORY_NODES * MAX_CELL_VALUE; // All cells with max value
    const float MIN_OPPONENT_DIST = 1.0f;

    reader_lock(sync);

    int x = gs->players[id].x;
    int y = gs->players[id].y;

    int best_dir = -1;
    float best_score = -1e9f;

    for (int d = 0; d < 8; ++d) {
        int nx = x + DIRS[d][0];
        int ny = y + DIRS[d][1];
        if (!in_bounds(gs, nx, ny)) continue;

        int v = get_cell(gs, nx, ny);
        if (!is_free_cell(v)) continue;

        float score = 0.0f;

        score += W_REWARD * ((float)v / MAX_CELL_VALUE);

        int territory_value = 0;
        int pot = territory_potential(gs, nx, ny, 20, 400, &territory_value);
        score += W_TERRITORY * ((float)pot / MAX_TERRITORY_NODES);
        
        // Normalize territory value to [0, W_TERRITORY_VAL]
        score += W_TERRITORY_VAL * ((float)territory_value / MAX_TERRITORY_VALUE);

        // Normalize opponent distance penalty to [0, W_NEAR_OPP]
        int dmin = min_chebyshev_to_opponent(gs, id, nx, ny);
        if (dmin > 0) {
            // Invert distance so closer opponents give higher penalty
            float proximity_factor = MIN_OPPONENT_DIST / (float)dmin;
            if (proximity_factor > 1.0f) proximity_factor = 1.0f;
            score -= W_NEAR_OPP * proximity_factor;
        }

        if (score > best_score) {
            best_score = score;
            best_dir = d;
        }
    }

    if (best_dir < 0) {
        reader_unlock(sync);
        return -1;
    }

    move[0] = DIRS[best_dir][0];
    move[1] = DIRS[best_dir][1];

    reader_unlock(sync);
    return 0;
}

int choose_best_move_naive(int *move, const game_state_t *gs, sync_t *sync, int id) {
    reader_lock(sync);
    int x = gs->players[id].x;
    int y = gs->players[id].y;

    for (int k = 0; k < 8; ++k) {
        int nx = x + DIRS[k][0];
        int ny = y + DIRS[k][1];
        if (in_bounds(gs, nx, ny) && is_free_cell(get_cell(gs, nx, ny))){
            move[0] = DIRS[k][0];
            move[1] = DIRS[k][1];
            reader_unlock(sync);
            return 0;
        }
    }
    reader_unlock(sync);
    return -1;
}
