// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef MASTER_H
#define MASTER_H

#include "args.h"

#define WIDTH_DEFAULT 10
#define HEIGHT_DEFAULT 10
#define DELAY_DEFAULT 200
#define TIMEOUT_DEFAULT 10

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

/**
 * Closes all file descriptors in the provided array
 * @param fds: array of file descriptor pairs
 * @param num_players: number of players (size of fds array)
 */
void close_fds(int fds[][2], int num_players);

/**
 * Initializes the args structure with default values
 * @param args: pointer to the args structure to initialize
 */
void initialize_default_args(args_t* args);

/**
 * Initializes the game state based on provided arguments and number of players
 * @param gs: pointer to the game state to initialize
 * @param args: pointer to the args structure with game settings
 * @param num_players: number of players in the game
 */
void init_game_state(game_state_t* gs, args_t* args, int num_players);

/**
 * Creates a view process to visualize the game state
 * @param view_path: path to the view executable
 * @param width_s: string representation of the board width
 * @param height_s: string representation of the board height
 * @return PID of the created view process, or -1 on failure
 */
pid_t create_view_process(const char* view_path, const char* width_s, const char* height_s);

/**
 * Creates a player process to participate in the game
 * @param player_path: path to the player executable
 * @param width_s: string representation of the board width
 * @param height_s: string representation of the board height
 * @param pipe_fd: array of two integers representing the pipe file descriptors (pipe_fd[0] for reading, pipe_fd[1] for writing)
 * @return PID of the created player process, or -1 on failure
 */
pid_t create_player_process(const char* player_path, const char* width_s, const char* height_s, int pipe_fd[MAX_PLAYERS][2], int player_idx);

/**
 * Starts the view process by signaling it to begin drawing
 * @param sync: pointer to the synchronization structure
 */
void start_view (sync_t* sync);

/**
 * Main gameplay loop handling player turns, timeouts, and game state updates
 * @param gs: pointer to the game state
 * @param sync: pointer to the synchronization structure
 * @param args: pointer to the args structure with game settings
 * @param fds: array of file descriptor pairs for player communication
 * @param num_players: number of players in the game
 * @param max_fd: maximum file descriptor value for select()
 */
void play(game_state_t* gs, sync_t* sync, const args_t* args, int fds[][2], int num_players, int max_fd);

/**
 * Checks if the game has timed out and marks it as finished if so
 * @param gs: pointer to the game state
 * @param sync: pointer to the synchronization structure
 * @param args: pointer to the args structure with timeout setting
 * @param last_successful_move_time: timestamp of the last successful move
 */
void check_timeout_and_finish(game_state_t* gs, sync_t* sync, const args_t* args, time_t last_successful_move_time);

/**
 * Checks if all players are blocked and marks the game as finished if so
 * @param gs: pointer to the game state
 * @param sync: pointer to the synchronization structure
 */
void check_all_blocked_and_finish(game_state_t* gs, sync_t* sync);

/**
 * Handles a player event (move) and updates game state accordingly
 * @param player_idx: index of the player making the move
 * @param gs: pointer to the game state
 * @param sync: pointer to the synchronization structure
 * @param player_fd: file descriptor to read the player's move from
 * @param last_successful_move_time: pointer to timestamp of last successful move
 */
void handle_player_event(int player_idx, game_state_t* gs, sync_t* sync, int player_fd, time_t* last_successful_move_time);

#endif //MASTER_H
