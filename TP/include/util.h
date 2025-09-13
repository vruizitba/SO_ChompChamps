// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <time.h>
#include "common.h"

static const int DIRS[8][2] = {
    {0,-1},   // UP
    {1,-1},   // UP_RIGHT
    {1,0},    // RIGHT
    {1,1},    // DOWN_RIGHT
    {0,1},    // DOWN
    {-1,1},   // DOWN_LEFT
    {-1,0},   // LEFT
    {-1,-1}   // UP_LEFT
};

/**
 * Converts a direction vector to an unsigned char (0-7)
 * Direction mapping (clockwise from top):
 * 0: North     (0, -1)
 * 1: NorthEast (1, -1)
 * 2: East      (1,  0)
 * 3: SouthEast (1,  1)
 * 4: South     (0,  1)
 * 5: SouthWest (-1, 1)
 * 6: West      (-1, 0)
 * 7: NorthWest (-1,-1)
 * 
 * @param direction: int vector [2] representing (x, y) direction
 * @return: unsigned char from 0-7, or 255 if invalid direction
 */
unsigned char direction_to_char(const int direction[2]);

/**
 * Checks if a cell value represents a free chocolate piece
 * @param cell_value: value from the game board
 * @return: true if cell is free (1-9), false otherwise
 */
bool is_free_cell(int cell_value);

/**
 * Checks if coordinates are within game board bounds
 * @param gs: pointer to game state
 * @param x: x coordinate
 * @param y: y coordinate
 * @return: true if coordinates are valid, false otherwise
 */
bool in_bounds(const game_state_t *gs, int x, int y);

/**
 * Safely gets a cell value from the game board
 * @param gs: pointer to game state
 * @param x: x coordinate
 * @param y: y coordinate
 * @return: cell value at the given coordinates
 */
int get_cell(const game_state_t *gs, int x, int y);

/**
 * Counts the number of free neighboring cells around a position
 * @param gs: pointer to game state
 * @param x: x coordinate
 * @param y: y coordinate
 * @return: number of free neighbors (0-8)
 */
int count_free_neighbors(const game_state_t *gs, int x, int y);

/**
 * Validates if a move is legal for a player
 * @param player_pos: player index
 * @param new_x: target x coordinate
 * @param new_y: target y coordinate  
 * @param gs: game state
 * @return: true if move is valid, false otherwise
 */
bool is_valid_move(int player_pos, int new_x, int new_y, game_state_t* gs);

/**
 * Calculates the time difference in milliseconds between two timespecs
 * @param start: starting timespec
 * @param end: ending timespec
 * @return: difference in milliseconds
 */
long calculate_time_diff_ms(struct timespec start, struct timespec end);

#endif //UTIL_H
