#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"

/**
 * Finds the player ID (index) in the game state based on process ID
 * @param gs: pointer to the game state
 * @param pid: process ID to search for
 * @return: player index (0 to MAX_PLAYERS-1) if found, -1 if not found
 */
int find_player_id(const game_state_t *gs, pid_t pid);

#endif //PLAYER_H
