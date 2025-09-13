// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"

/**
 * Finds the player ID (index) in the game state based on process ID
 * @param gs: pointer to the game state
 * @param pid: process ID to search for
 * @return: player index (0 to MAX_PLAYERS-1) if found, -1 if not found
 */
int find_player_id(const game_state_t *gs, pid_t pid, sync_t * sync);

#endif //PLAYER_H
