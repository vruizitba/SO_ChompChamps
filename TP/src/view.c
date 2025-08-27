// src/view.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>

#include "common.h"
#include "sync.h"
#include "util.h"   // in_bounds(...), DIRS[], etc.

/* ===== Helpers de tablero/estado ===== */
static inline int board_cell(const game_state_t *gs, int x, int y) {
    return gs->board[y * gs->width + x];
}
static inline int is_free_cell_v(int v) { return v >= 1 && v <= 9; }
static inline int owner_from_v(int v)   { return -v; } // v<=0 => -id

/* ===== Layout ===== */
typedef struct {
    int rows, cols;
    int info_h;   // alto panel
    int board_y;  // origen Y del tablero (en ncurses)
    int board_x;  // origen X del tablero (en ncurses)
} layout_t;

static void compute_layout(layout_t *ly, const game_state_t *gs) {
    getmaxyx(stdscr, ly->rows, ly->cols);
    ly->info_h = 5 + (int)gs->num_players;   // cabecera + 1 línea por jugador
    ly->board_y = ly->info_h + 1;
    ly->board_x = 2;
    if (ly->board_y >= ly->rows - 1) ly->board_y = ly->rows - 1;
}

/* ===== UI + colores ===== */
static void ui_init(void) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, FALSE);
    start_color();
#ifdef NCURSES_VERSION
    use_default_colors();
#endif
}

static void ui_end(void) { endwin(); }

static void init_colors(void) {
    // jugadores (1..9)
    init_pair(1, COLOR_BLACK,   COLOR_RED);
    init_pair(2, COLOR_BLACK,   COLOR_GREEN);
    init_pair(3, COLOR_BLACK,   COLOR_YELLOW);
    init_pair(4, COLOR_BLACK,   COLOR_BLUE);
    init_pair(5, COLOR_BLACK,   COLOR_MAGENTA);
    init_pair(6, COLOR_BLACK,   COLOR_CYAN);
    init_pair(7, COLOR_BLACK,   COLOR_WHITE);
    init_pair(8, COLOR_WHITE,   COLOR_RED);
    init_pair(9, COLOR_WHITE,   COLOR_BLUE);

    // auxiliares
    init_pair(20, COLOR_WHITE,  -1);          // texto normal
    init_pair(21, COLOR_CYAN,   -1);          // títulos
    init_pair(22, COLOR_YELLOW, -1);          // números de recompensa
    init_pair(23, COLOR_WHITE,  COLOR_BLACK); // borde tablero
}

/* ===== Dibujo ===== */
static void draw_header(const game_state_t *gs) {
    attron(COLOR_PAIR(21) | A_BOLD);
    mvprintw(0, 2, "ChompChamps  %ux%u  players:%u  finished:%d",
             gs->width, gs->height, gs->num_players, gs->finished);
    attroff(COLOR_PAIR(21) | A_BOLD);
}

static void draw_players_info(const game_state_t *gs) {
    // ordenar por score desc (N<=9, simple burbujeo)
    int idx[MAX_PLAYERS];
    for (unsigned i = 0; i < gs->num_players; ++i) idx[i] = (int)i;
    for (unsigned a = 0; a + 1 < gs->num_players; ++a)
        for (unsigned b = a + 1; b < gs->num_players; ++b)
            if (gs->players[idx[b]].score > gs->players[idx[a]].score) {
                int t = idx[a]; idx[a] = idx[b]; idx[b] = t;
            }

    mvprintw(2, 2, "Players:");
    for (unsigned r = 0; r < gs->num_players; ++r) {
        int i = idx[r];
        int pair = 1 + (i % 9);
        // muestrario de color
        attron(COLOR_PAIR(pair) | A_BOLD);
        mvaddstr(3 + (int)r, 2, "  ");
        attroff(COLOR_PAIR(pair) | A_BOLD);

        // nombre + stats (ajustá si tus campos difieren)
        attron(COLOR_PAIR(20));
        mvprintw(3 + (int)r, 5, "%s  sc:%u  ok:%u  bad:%u  pos:(%u,%u)%s",
                 gs->players[i].name,
                 gs->players[i].score,
                 gs->players[i].valids,
                 gs->players[i].invalids,
                 gs->players[i].x, gs->players[i].y,
                 gs->players[i].blocked ? "  [blocked]" : "");
        attroff(COLOR_PAIR(20));
    }
}

static void draw_board_frame(const layout_t *ly, const game_state_t *gs) {
    int h = (int)gs->height, w = (int)gs->width;
    attron(COLOR_PAIR(23));
    for (int x = 0; x <= w + 1; ++x) {
        mvaddch(ly->board_y - 1, ly->board_x + x, (x==0||x==w+1) ? '+' : '-');
        mvaddch(ly->board_y + h, ly->board_x + x, (x==0||x==w+1) ? '+' : '-');
    }
    for (int y = 0; y < h; ++y) {
        mvaddch(ly->board_y + y, ly->board_x - 1, '|');
        mvaddch(ly->board_y + y, ly->board_x + w, '|');
    }
    attroff(COLOR_PAIR(23));
}

static void draw_board_cells(const layout_t *ly, const game_state_t *gs) {
    int h = (int)gs->height, w = (int)gs->width;

    // celdas
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int v = board_cell(gs, x, y);
            if (is_free_cell_v(v)) {
                attron(COLOR_PAIR(22) | A_DIM);
                mvaddch(ly->board_y + y, ly->board_x + x, '0' + (v % 10));
                attroff(COLOR_PAIR(22) | A_DIM);
            } else {
                int id = owner_from_v(v);
                int pair = 1 + (id % 9);
                attron(COLOR_PAIR(pair));
                mvaddch(ly->board_y + y, ly->board_x + x, ' ');
                attroff(COLOR_PAIR(pair));
            }
        }
    }

    // cabezas actuales con sombreado (bloque sólido)
    for (unsigned i = 0; i < gs->num_players; ++i) {
        int px = gs->players[i].x;
        int py = gs->players[i].y;
        if (!in_bounds(gs, px, py)) continue;
        int pair = 1 + ((int)i % 9);
        attron(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
        mvaddch(ly->board_y + py, ly->board_x + px, ACS_BLOCK);
        attroff(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
    }
}

/* Cubo “rodando”: 3 frames breves con ACS */
static void animate_cube_roll(const layout_t *ly,
                              int from_x, int from_y,
                              int to_x, int to_y,
                              int pair) {
    const chtype frames[3] = { ACS_CKBOARD, ACS_DIAMOND, ACS_BLOCK };
    int fx = ly->board_x + from_x, fy = ly->board_y + from_y;
    int tx = ly->board_x + to_x,   ty = ly->board_y + to_y;

    attron(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
    mvaddch(fy, fx, frames[0]);
    attroff(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
    refresh(); napms(18);

    int mx = fx + ((tx - fx) > 0 ? 1 : ((tx - fx) < 0 ? -1 : 0));
    int my = fy + ((ty - fy) > 0 ? 1 : ((ty - fy) < 0 ? -1 : 0));
    attron(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
    mvaddch(my, mx, frames[1]);
    attroff(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
    refresh(); napms(18);

    attron(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
    mvaddch(ty, tx, frames[2]);
    attroff(COLOR_PAIR(pair) | A_BOLD | A_STANDOUT);
    refresh(); napms(18);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <width> <height>\n", argv[0]);
        return 1;
    }

    game_state_t *gs = attach_game_state_shm_readonly();
    if (!gs) { perror("attach game_state"); return 1; }
    sync_t *sync = attach_sync_shm();
    if (!sync) { perror("attach sync"); return 1; }

    ui_init();
    init_colors();

    layout_t ly;
    int last_have = 0;
    int last_x[MAX_PLAYERS], last_y[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; ++i) last_x[i] = last_y[i] = -1;

    for (;;) {
        /* Espera a que el master anuncie cambios */
        sem_wait(&sync->drawing_signal);

        reader_lock(sync);

        int finished = gs->finished;
        compute_layout(&ly, gs);
        clear();

        draw_header(gs);
        draw_players_info(gs);
        draw_board_frame(&ly, gs);
        draw_board_cells(&ly, gs);

        /* Animar solo jugadores que cambiaron de celda desde el último frame */
        if (last_have) {
            for (unsigned i = 0; i < gs->num_players; ++i) {
                int cx = gs->players[i].x;
                int cy = gs->players[i].y;
                if (last_x[i] >= 0 && last_y[i] >= 0 &&
                    (cx != last_x[i] || cy != last_y[i])) {
                    int pair = 1 + ((int)i % 9);
                    animate_cube_roll(&ly, last_x[i], last_y[i], cx, cy, pair);
                }
            }
        }
        for (unsigned i = 0; i < gs->num_players; ++i) {
            last_x[i] = gs->players[i].x;
            last_y[i] = gs->players[i].y;
        }
        last_have = 1;

        reader_unlock(sync);

        refresh();

        /* Avisar al master que ya imprimimos */
        sem_post(&sync->not_drawing_signal);

        if (finished) break;

        // escape manual (no bloquea el handshake)
        nodelay(stdscr, TRUE);
        int ch = getch();
        nodelay(stdscr, FALSE);
        if (ch == 'q' || ch == 'Q') break;
    }

    ui_end();
    return 0;
}