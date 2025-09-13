// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef AI_H
#define AI_H

#include "common.h"

/**
 * Chooses the best move for a player using primitive AI logic
 * @param move: output array [2] to store the chosen direction vector
 * @param gs: pointer to the game state
 * @param sync: pointer to synchronization structures  
 * @param id: player ID
 * @return: 0 if a valid move is found, -1 if no moves available
 */
int choose_best_move(int *move, const game_state_t *gs, sync_t *sync, int id);

/**
 * Chooses the best move for a player using naive/primitive AI logic
 * Simple algorithm that selects the first valid move found in directional order
 * @param move: output array [2] to store the chosen direction vector
 * @param gs: pointer to the game state
 * @param sync: pointer to synchronization structures  
 * @param id: player ID
 * @return: 0 if a valid move is found, -1 if no moves available
 */
int choose_best_move_naive(int *move, const game_state_t *gs, sync_t *sync, int id);

#endif //AI_H
