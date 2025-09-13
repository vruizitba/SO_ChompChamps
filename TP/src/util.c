// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "util.h"
#include "common.h"
#include <fcntl.h>
#include <stdlib.h>

unsigned char direction_to_char(const int direction[2]) {
    for (int i = 0; i < 8; i++) {
        if (DIRS[i][0] == direction[0] && DIRS[i][1] == direction[1]) {
            return (unsigned char)i;
        }
    }
    return 255;
}

bool is_free_cell(int cell_value) {
    return (cell_value > 0 && cell_value <= 9);
}

bool in_bounds(const game_state_t *gs, int x, int y) {
    return x >= 0 && y >= 0 && x < (int)gs->width && y < (int)gs->height;
}

int get_cell(const game_state_t *gs, int x, int y) {
    return gs->board[y * gs->width + x];
}

int count_free_neighbors(const game_state_t *gs, int x, int y) {
    int count = 0;
    for (int k = 0; k < 8; k++) {
        int nx = x + DIRS[k][0];
        int ny = y + DIRS[k][1];
        if (in_bounds(gs, nx, ny) && is_free_cell(get_cell(gs, nx, ny))) {
            count++;
        }
    }
    return count;
}

bool is_valid_move(int player_pos, int new_x, int new_y, game_state_t* gs) {
    if (new_x < 0 || new_x >= gs->width || new_y < 0 || new_y >= gs->height) {
        return false;
    }
    
    int current_x = gs->players[player_pos].x;
    int current_y = gs->players[player_pos].y;
    
    int dx = new_x - current_x;
    int dy = new_y - current_y;
    
    if (abs(dx) > 1 || abs(dy) > 1 || (dx == 0 && dy == 0)) {
        return false;
    }
    
    int cell_value = gs->board[new_y * gs->width + new_x];
    return is_free_cell(cell_value);
}

long calculate_time_diff_ms(struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    return seconds * 1000 + nanoseconds / 1000000;
}
