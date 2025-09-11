#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <ncurses.h>
#include <string.h>

#include "common.h"
#include "sync.h"
#include "util.h"

/* ========= helpers ========= */

static inline int owner_from_v(int v){ return -v; }

/* ========= layout ========= */
typedef struct {
    int rows, cols;
    int info_h;           // panel superior
    int board_y, board_x; // origen tablero
    int xmul, ymul;       // “multipixel” por celda
    int w_real, h_real;   // rectángulo del tablero (incluye borde)
} layout_t;

static void compute_layout(layout_t *ly, const game_state_t *gs) {
    getmaxyx(stdscr, ly->rows, ly->cols);

    ly->info_h = 3 + (int)gs->num_players;
    if (ly->info_h > ly->rows - 4)
        ly->info_h = ly->rows - 4;
    if (ly->info_h < 3)
        ly->info_h = 3;

    int avail_h = ly->rows - ly->info_h - 2; // respiración
    int avail_w = ly->cols - 2;

    ly->xmul = (avail_w - 2) / (int)gs->width;   // -2: borde
    ly->ymul = (avail_h - 2) / (int)gs->height;  // -2: borde
    if (ly->xmul < 1) ly->xmul = 1;
    if (ly->ymul < 1) ly->ymul = 1;

    // celdas más grandes que antes
    if (ly->xmul > 6) ly->xmul = 6;
    if (ly->ymul > 3) ly->ymul = 3;

    ly->w_real = (int)gs->width * ly->xmul + 2;
    ly->h_real = (int)gs->height * ly->ymul + 2;

    // centrar
    ly->board_x = (ly->cols - ly->w_real) / 2;
    if (ly->board_x < 0) ly->board_x = 0;
    ly->board_y = ly->info_h;
    if (ly->board_y + ly->h_real > ly->rows) {
        ly->board_y = ly->rows - ly->h_real;
        if (ly->board_y < 0) ly->board_y = 0;
    }
}

/* ========= UI / colores ========= */
static int g_use_256 = 0;

static void ui_init(void){
    if (!getenv("TERM")) setenv("TERM", "xterm-256color", 1);
    setlocale(LC_ALL,"");
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    start_color();
#ifdef NCURSES_VERSION
    use_default_colors();
#endif
    g_use_256 = (COLORS >= 256);
}
static void ui_end(void){ endwin(); }

/* Paleta sobria:
   - Recompensas: blanco tenue (sin amarillos/naranjas chillones).
   - Por jugador: territorio vs cabeza (misma familia, cabeza más viva).
   - Con 256 colores: usamos índices xterm-256 para tonos oscuros/claros.
   - Fallback 8 colores: evitamos A_STANDOUT para no “apagar” el fondo.
*/
static void init_colors(void){
    // Texto/estáticos
    init_pair(1,  COLOR_WHITE,   -1);          // recompensas (dim)
    init_pair(2,  COLOR_CYAN,    -1);          // títulos
    init_pair(3,  COLOR_BLACK,  COLOR_WHITE); // marco

    if (g_use_256) {
        // BG territory (oscuro) y BG head (brillante) por jugador (0..8)
        // Elegidos del cubo 256 de xterm para mantener familia y saturación.
        const short TERR_BG[9] = { 17, 22, 52,  90,  30, 24, 250, 33,  93 }; // navy, dark-green, dark-red, dark-magenta, dark-cyan, teal, grey, blue, violet
        const short HEAD_BG[9] = { 39, 46,196, 201,  51, 45, 255, 69, 207 }; // bright cyan, lime, bright red, hot magenta, bright cyan, spring-cyan, white, dodger blue, bright violet

        for (int i=0;i<9;i++){
            init_pair(30 + 2*i, COLOR_BLACK, TERR_BG[i]); // territorio
            init_pair(31 + 2*i, COLOR_BLACK, HEAD_BG[i]); // cabeza
        }
    } else {
        // Fallback a 8 colores:
        const short TERR_BG8[9] = {
            COLOR_BLUE, COLOR_GREEN, COLOR_RED, COLOR_MAGENTA,
            COLOR_CYAN, COLOR_BLUE,  COLOR_WHITE, COLOR_BLUE, COLOR_MAGENTA
        };
        const short HEAD_BG8[9] = {
            COLOR_CYAN, COLOR_GREEN, COLOR_MAGENTA, COLOR_WHITE,
            COLOR_WHITE, COLOR_CYAN,  COLOR_WHITE,  COLOR_WHITE,  COLOR_WHITE
        };
        for (int i=0;i<9;i++){
            init_pair(30 + 2*i, COLOR_BLACK, TERR_BG8[i]);
            init_pair(31 + 2*i, COLOR_BLACK, HEAD_BG8[i]);
        }
    }
}

/* ========= dibujo ========= */
static void draw_header(const game_state_t *gs){
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(0, 2, "ChompChamps   board:%ux%u   players:%u   finished:%d",
             gs->width, gs->height, gs->num_players, gs->finished);
    attroff(COLOR_PAIR(2) | A_BOLD);
    mvprintw(2, 2, "Name           Score  OK   BAD   Pos        State");
}

static void draw_players_info(const game_state_t *gs){
    for (unsigned i=0;i<gs->num_players;i++){
        int row = 3 + (int)i;
        int tag = 30 + (int)i*2;
        attron(COLOR_PAIR(tag));
        mvaddstr(row, 2, "  ");
        attroff(COLOR_PAIR(tag));
        mvprintw(row, 5, "%-12s  %-5u %-4u %-5u (%3u,%3u)  %s",
                 gs->players[i].name,
                 gs->players[i].score,
                 gs->players[i].valids,
                 gs->players[i].invalids,
                 gs->players[i].x, gs->players[i].y,
                 gs->players[i].blocked ? "blocked" : "ok");
    }
}

static void draw_board_frame(const layout_t *ly){
    int x0 = ly->board_x, y0 = ly->board_y;
    int w  = ly->w_real,  h  = ly->h_real;

    attron(COLOR_PAIR(3));
    mvaddch(y0,         x0,         ACS_ULCORNER);
    mvaddch(y0,         x0 + w - 1, ACS_URCORNER);
    mvaddch(y0 + h - 1, x0,         ACS_LLCORNER);
    mvaddch(y0 + h - 1, x0 + w - 1, ACS_LRCORNER);
    for (int x=1; x<w-1; x++){
        mvaddch(y0,         x0 + x, ACS_HLINE);
        mvaddch(y0 + h - 1, x0 + x, ACS_HLINE);
    }
    for (int y=1; y<h-1; y++){
        mvaddch(y0 + y, x0,         ACS_VLINE);
        mvaddch(y0 + y, x0 + w - 1, ACS_VLINE);
    }
    attroff(COLOR_PAIR(3));
}

static void fill_cell_rect(const layout_t *ly, int cx, int cy, int pair, chtype center_char, int emphasize_bold){
    int sx = ly->board_x + 1 + cx * ly->xmul;
    int sy = ly->board_y + 1 + cy * ly->ymul;

    if (emphasize_bold) attron(A_BOLD);          // NO usamos A_STANDOUT (apaga el bg en algunas TTY)
    attron(COLOR_PAIR(pair));

    for (int r=0; r<ly->ymul; r++){
        for (int c=0; c<ly->xmul; c++){
            int is_center = (r == ly->ymul/2) && (c == ly->xmul/2);
            mvaddch(sy + r, sx + c, is_center ? center_char : ' ');
        }
    }

    attroff(COLOR_PAIR(pair));
    if (emphasize_bold) attroff(A_BOLD);
}

static void draw_board(const layout_t *ly, const game_state_t *gs){
    // terreno / recompensas
    for (int y=0; y<(int)gs->height; y++){
        for (int x=0; x<(int)gs->width; x++){
            int v = get_cell(gs, x, y);
            if (is_free_cell(v)) {
                attron(COLOR_PAIR(1) | A_DIM);
                fill_cell_rect(ly, x, y, 1, (chtype)('0' + (v % 10)), 0);
                attroff(COLOR_PAIR(1) | A_DIM);
            } else {
                int id   = owner_from_v(v);
                int base = 30 + id*2;    // territorio
                fill_cell_rect(ly, x, y, base, ' ', 0);
            }
        }
    }
    // cabezas (rellenas, mismo tono pero más vivo/saturado)
    for (unsigned i=0; i<gs->num_players; i++){
        int px = gs->players[i].x, py = gs->players[i].y;
        if (!in_bounds(gs, px, py)) continue;
        int head = 31 + (int)i*2;        // par de cabeza
        fill_cell_rect(ly, px, py, head, ' ', 1); // relleno completo; bold para “levantar” el tono
    }
}

/* ========= main loop ========= */
int main(int argc, char **argv){
    (void)argc; (void)argv;

    game_state_t *gs = attach_game_state_shm_readonly();
    if (!gs){ perror("attach game_state"); return 1; }
    sync_t *sync = attach_sync_shm();
    if (!sync){ perror("attach sync"); return 1; }

    ui_init();
    init_colors();

    layout_t ly;

    bool finished;

    do {
        sem_wait(&sync->drawing_signal);
        reader_lock(sync);

        finished = gs->finished;
        compute_layout(&ly, gs);
        clear();

        draw_header(gs);
        draw_players_info(gs);
        draw_board_frame(&ly);
        draw_board(&ly, gs);

        reader_unlock(sync);
        refresh();
        sem_post(&sync->not_drawing_signal);

    } while (!finished);


    nodelay(stdscr, FALSE); // aseguramos modo bloqueante
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(ly.board_y + ly.h_real + 1, ly.board_x,
             "Juego finalizado. Pulse cualquier tecla para salir...");
    attroff(COLOR_PAIR(2) | A_BOLD);
    refresh();
    getch();

    ui_end();
    return 0;
}
