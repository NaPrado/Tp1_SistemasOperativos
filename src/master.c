#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

enum params_default {DEF_WIDTH = 10, DEF_HEIGHT = 10, DEF_DELAY = 200, DEF_TIMEOUT = 10};

#define FLAG_WIDTH "-w"
#define FLAG_HEIGHT "-h"
#define FLAG_DELAY "-d"
#define FLAG_SEED "-s"
#define FLAG_VIEW "-v"
#define FLAG_TIMEOUT "-t"
#define FLAG_PLAYER "-p"

#define MAX_NUM_PLAYERS 9

void setParams(int argc, char const *argv[], int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]);
int checkParams(int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]);

int main(int argc, char const *argv[]) {

    int num_params[5] = {0};
    char * view = NULL;
    char * players[MAX_NUM_PLAYERS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    setParams(argc, argv, num_params, &view, players);

    if (checkParams(num_params, &view, players) == 1) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Width: %d\n", num_params[0]);
    printf("Height: %d\n", num_params[1]);
    printf("Delay: %d\n", num_params[2]);
    printf("Seed: %d\n", num_params[3]);
    printf("Timeout: %d\n", num_params[4]);
    printf("View: %s\n", view);
    for (int i = 0; i < MAX_NUM_PLAYERS; i++) {
        if (players[i] != NULL) {
            printf("Player %d: %s\n", i, players[i]);
        }
    }
    
    
    return 0;
}

void setParams(int argc, char const *argv[], int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]) {
    int i = 1;
    while (i+1 < argc) {
        if (strcmp(argv[i], FLAG_WIDTH) == 0) {
            num_params[0] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_HEIGHT) == 0) {
            num_params[1] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_DELAY) == 0) {
            num_params[2] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_SEED) == 0) {
            num_params[3] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_VIEW) == 0) {
            *view = argv[i+1];
            i += 2;
        } else if (strcmp(argv[i], FLAG_TIMEOUT) == 0) {
            num_params[4] = atoi(argv[i+1]);
            i += 2;
        } else if (strcmp(argv[i], FLAG_PLAYER) == 0) {
            int j = 0;
            i++;
            while (i < argc && strcmp(argv[i], FLAG_DELAY) != 0 && strcmp(argv[i], FLAG_HEIGHT) != 0 && strcmp(argv[i], FLAG_WIDTH) != 0 && strcmp(argv[i], FLAG_SEED) != 0 && strcmp(argv[i], FLAG_VIEW) != 0 && strcmp(argv[i], FLAG_TIMEOUT) != 0 && strcmp(argv[i], FLAG_PLAYER) != 0) {
                players[j++] = argv[i++];
            }
        }
    }
}

int checkParams(int * num_params, char ** view, char * players[MAX_NUM_PLAYERS]) {
    if (num_params[0] <= DEF_WIDTH) {
        num_params[0] = DEF_WIDTH;
    }
    if (num_params[1] <= DEF_HEIGHT) {
        num_params[1] = DEF_HEIGHT;
    }
    if (num_params[2] <= 0) {
        num_params[2] = DEF_DELAY;
    }
    if (num_params[3] <= 0) {
        num_params[3] = time(NULL);
    }
    if (num_params[4] <= 0) {
        num_params[4] = DEF_TIMEOUT;
    }
    if (players[0] == NULL) {
        return 1;
    }
    return 0;
}
