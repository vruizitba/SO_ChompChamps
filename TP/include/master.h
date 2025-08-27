
#ifndef MASTER_H
#define MASTER_H

#include "common.h"
#include "args.h"
#include <sys/types.h>

#define WIDTH_DEFAULT 10
#define HEIGHT_DEFAULT 10
#define DELAY_DEFAULT 200
#define TIMEOUT_DEFAULT 10
#define MAX_BOARD_VALUE 9
#define MIN_BOARD_VALUE 1

/**
 * Prints the winners of the game after applying tiebreaker rules
 * @param gs: pointer to the game state
 */
void print_winners(game_state_t* gs);

/**
 * Waits for all player processes and the view process to finish
 * @param gs: pointer to the game state
 * @param view: view process PID
 */
void wait_all(game_state_t* gs, pid_t view);

/**
 * Sets valid positions for a player on the game board
 * @param gs: pointer to the game state
 * @param player_pos: player index
 */
void set_valid_positions(game_state_t* gs, int player_pos);

#endif //MASTER_H
