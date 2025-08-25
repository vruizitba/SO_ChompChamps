
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include "master.h"

#include "sync.h"
#include "common.h"

void inline static set_valid_positions(unsigned short* x, unsigned short* y); //hacer

void inline static set_nonblocking(int fd);

void inline static writer_lock(); //hacer

void inline static writer_unlock(); //hacer

void inline static set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(1);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int num_players = 0;
    int delay = DELAY_DEFAULT;
    int seed = time(NULL);
    int timeout = TIMEOUT_DEFAULT;
    char* view = NULL;
    int width = WIDTH_DEFAULT;
    int height = HEIGHT_DEFAULT;

    char* players_bins[MAX_PLAYERS];

    for (int i = 1; i < argc; i++) {

        if (argv[i] != NULL && argv[i][0] == '-') {

            char* arg = argv[i + 1];
            char type_arg = argv[i][1];

            switch (type_arg) { // falta chequeos
                case 'w':
                    width = atoi(arg);
                    break;
                case 'h':
                    height = atoi(arg);
                    break;
                case 'p':
                    while (argv[i + 1 + num_players][0] == '.' || argv[i + 1 + num_players][0] == '/') {
                        players_bins[num_players] = argv[i + 1 + num_players++];
                    }
                    break;
                case 'd':
                    delay = atoi(arg);
                    break;
                case 's':
                    seed = atoi(arg);
                    break;
                case 't':
                    timeout = atoi(arg);
                    break;
                case 'v':
                    view = arg;
                    break;
                default:
                    perror("Not valid argument");
            }
        }
    }

    game_state_t* gs = allocate_game_state_shm(width, height);

    srand(seed);

    gs->width = width;
    gs->height = height;
    gs->num_players = num_players;
    gs->finished = false;

    for (int i = 0; i < height * width; i++) {
        gs->board[i] = ((rand() % MAX_BOARD_VALUE) + MIN_BOARD_VALUE);
    }

    sync_t* sync = allocate_sync_shm();
    init_sync(sync);

    for (int i = 0; i < num_players; i++) {
        gs->players[i].blocked = false;
        gs->players[i].invalids = 0;
        sprintf(gs->players[i].name, "Player %d", i + 1);
        gs->players[i].score = 0;
        gs->players[i].valids = 0;
        set_valid_positions(&(gs->players[i].x), &(gs->players[i].y));
    }

    int fds[num_players][2];
    char width_s[6]; // ver bien si poner 6 u otra cosa
    char height_s[6];

    for (int i = 0; i < num_players; i++) {

        if (pipe(fds[i]) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t pid_p = fork();

        if (pid_p < 0) {
            perror("fork p");
            return 1;
        }

        if (pid_p == 0) {
            //codigo hijo
            if (dup2(fds[i][1], STDOUT_FILENO) == -1) {
                perror("dup2 p");
                _exit(1);
            }

            close(fds[i][0]);
            close(fds[i][1]);

            sprintf(width_s, "%d", width);
            sprintf(height_s, "%d", height);

            char *argv_c[] = { players_bins[i], width_s, height_s, NULL };
            execve(players_bins[i], argv_c, NULL);
            perror("exec player");
            _exit(1);
        }

        //codigo padre
        gs->players[i].pid = pid_p;
        close(fds[i][1]);
        set_nonblocking(fds[i][0]);
    }

    int i = 0;
    int blocked_players;
    time_t start_time = time(NULL);
    int n;
    char mov[1];

    while (!gs->finished) {
		n = read(fds[i][0], mov, 1);
        if (n == 0) {
          gs->players[i].blocked = true;
        } else if (n == 1) {
          // procesa movimiento
          writer_lock();
          // bla
          writer_unlock();
          // bla?
          // verificar si es valido, y en caso de serlo, actualizar time
        }

      	if (difftime(time(NULL), start_time) > timeout) {
          gs->finished = true;
        }

      	blocked_players = 0;

        if (!gs->finished) {
            for (int j = 0; j < num_players; j++) {
                if (gs->players[j].blocked) {
                    blocked_players++;
                }
            }
            if (blocked_players == num_players) {
                gs->finished = true;
            }
        }

        i = (i + 1) % num_players;
    }


}