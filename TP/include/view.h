#ifndef VIEW_H
#define VIEW_H

#include "common.h"

typedef struct {
    int screen_width, screen_height;
    int board_start_x, board_start_y;
    int cell_width, cell_height;
    int board_width, board_height;
} layout_t;

/**
 * Initializes the ncurses UI system
 */
void ui_init(void);

/**
 * Ends the ncurses UI system
 */
void ui_end(void);

/**
 * Initializes color pairs for the display
 */
void init_colors(void);

/**
 * Computes the layout parameters for displaying the game board
 * @param ly: pointer to layout structure to fill
 * @param gs: pointer to the game state
 */
void compute_layout(layout_t *ly, const game_state_t *gs);

#endif //VIEW_H
