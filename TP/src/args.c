#include "args.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int parse_args(int argc, char **argv, args_t *args) {
    int player_count = 0;
    int opt;

    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
        case 'w':
            args->width = (unsigned short)atoi(optarg);
            break;
        case 'h':
            args->height = (unsigned short)atoi(optarg);
            break;
        case 'd':
            args->delay = atoi(optarg);
            break;
        case 't':
            args->timeout = atoi(optarg);
            break;
        case 's':
            args->seed = (unsigned int)atoi(optarg);
            break;
        case 'v':
            args->view_path = optarg;
            break;
        case 'p':
            args->player_paths[player_count++] = optarg;
            while (optind < argc && argv[optind][0] != '-') {
                if (player_count >= MAX_PLAYERS) {
                    fprintf(stderr, "Error: máximo %d jugadores\n", MAX_PLAYERS);
                    return -1;
                }
                args->player_paths[player_count++] = argv[optind++];
            }
            break;
        default:
            fprintf(stderr, "Uso: %s [-w width] [-h height] [-d delay] [-t timeout] "
                            "[-s seed] [-v view] -p player1 [player2 ...]\n", argv[0]);
            return -1;
        }
    }

    if (player_count == 0) {
        fprintf(stderr, "Error: debe especificar al menos un jugador con -p\n");
        return -1;
    }

    return player_count;
}
