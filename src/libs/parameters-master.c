#include "parameters-master.h"
#include "return-codes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define OPT_STRING "w:h:d:s:v:t:p:"

#define INICIAL_SLEEP_TIME 3

enum params_default {
    DEF_WIDTH = 10, 
    DEF_HEIGHT = 10, 
    DEF_DELAY = 200, 
    DEF_TIMEOUT = 10
};
#define DEF_SEED time(NULL)
#define DEF_PROCESS NULL

extern char *optarg;
extern int optind, opterr, optopt;

int set_params(int argc, char * const argv[], Tparameters * params) {
    int op;
    if (argc < 2) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        return ERROR;
    }
    while ((op = getopt(argc, argv, OPT_STRING)) != ERROR) {
        switch (op) {
            case 'w':
                if (atoi(optarg) >= 10) {
                    params->width = atoi(optarg);
                }
                break;
            case 'h':
                if (atoi(optarg) >= 10) {
                    params->height= atoi(optarg);
                }
                break;
            case 'd':
                if (atoi(optarg) > 0) {
                    params->delay = atoi(optarg);
                }
                break;
            case 's':
                if (atoi(optarg) > 0) {
                    params->seed = atoi(optarg);
                }
                break;
            case 'v':
                params->view = optarg;
                break;
            case 't':
                if (atoi(optarg) > 0) {
                    params->timeout = atoi(optarg);
                }
                break;
            case 'p': {
                int idx = optind - 1;
                params->amount_players = 0;
                while (argv[idx] != NULL && argv[idx][0] != '-') {
                    if (params->amount_players >= MAX_NUM_PLAYERS) {
                        fprintf(stderr, "Error: At most 9 players can be specified using -p");
                        return ERROR;
                    }
                    params->players[params->amount_players++] = argv[idx++];
                }
                break;
            }
            default:
                return ERROR;
        }
    }
    if (params->players[0] == NULL) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        return ERROR;
    }
    return SUCCESS;
}

Tparameters get_default_params() {
    Tparameters params = {
        .width = DEF_WIDTH,
        .height = DEF_HEIGHT,
        .delay = DEF_DELAY,
        .timeout = DEF_TIMEOUT,
        .seed = DEF_SEED,
        .view = DEF_PROCESS,
        .players = {0}, // inicializo todo en 0 (NULL)
        .amount_players = 0
    };
    return params;
}

void print_inicial_state(Tparameters params) {
    printf("\033[H\033[J");
    printf("width: %zu\n", params.width);
    printf("height: %zu\n", params.height);
    printf("delay: %zu\n", params.delay);
    printf("timeout: %zu\n", params.timeout);
    printf("seed: %zu\n", params.seed);
    printf("view: %s\n", params.view == NULL ? "-" : params.view);
    printf("num_players: %zu\n", params.amount_players);
    for (int i = 0; i < params.amount_players; i++) {
        printf("  %s\n", params.players[i]);
    }
    sleep(INICIAL_SLEEP_TIME);
}
