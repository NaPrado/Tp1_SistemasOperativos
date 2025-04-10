#include "processes-master.h"
#include "parameters-master.h"
#include "structures.h"
#include "return-codes.h"
#include <unistd.h>
#include <stdio.h>

#define MAX_STR_LENGTH 10

enum PIPES {
    READ_END = 0,
    WRITE_END = 1
};

int set_pipes(int fd[][2], int num_players) {
    for (int i = 0; i < num_players; i++) {
        if (pipe(fd[i]) == -1) {
            return ERROR;
        }
    }
    return SUCCESS;
}

int set_players_processes(Tparameters* params,game_status* game_state, int fd[][2]) {
    int pid = 0;
    for (int i = 0; i < params->amount_players; i++) {
        if ((pid = fork()) < 0) {
            perror("Error: player fork failed");
            return ERROR;
        } else if (pid == 0) { // hijo
            if (close(fd[i][READ_END]) == -1) {
                perror("Error: pipe setting");
                return ERROR;
            }
            if (dup2(fd[i][WRITE_END], STDOUT_FILENO) == -1) {
                perror("Error: pipe setting");
                return ERROR;
            }
            if (close(fd[i][WRITE_END]) == -1) {
                perror("Error: pipe setting");
                return ERROR;
            }

            char * new_argv[] = {NULL, NULL, NULL, NULL};
            char arg0[MAX_PLAYER_LENGTH] = {0};
            snprintf(arg0, 19, "%s", params->players[i]);
            char arg1[MAX_STR_LENGTH] = {0};
            snprintf(arg1, 9, "%ld", params->width);
            char arg2[MAX_STR_LENGTH] = {0};
            snprintf(arg2, 9, "%ld", params->height);
            new_argv[0] = arg0;
            new_argv[1] = arg1;
            new_argv[2] = arg2;
            new_argv[3] = NULL;

            execve(params->players[i], new_argv, NULL);
            return ERROR;
        } else { // padre
            if (close(fd[i][WRITE_END]) == -1) {
                return ERROR;
            }
            game_state->players[i].pid = pid;
        }
    }
    return SUCCESS;
}

int set_view_process(const char * view_name, size_t width, size_t height) {
    if (view_name == NULL) {
        return ERROR;
    }
    int pid = fork();
    if (pid < 0) {
        perror("Error: view fork failed");
        return ERROR;
    }
    if (pid == 0) { // hijo
        char * new_argv[] = {NULL, NULL, NULL, NULL};
        char arg0[MAX_PLAYER_LENGTH] = {0};
        snprintf(arg0, 19, "%s", view_name);
        char arg1[MAX_STR_LENGTH] = {0};
        snprintf(arg1, 9, "%zu", width);
        char arg2[MAX_STR_LENGTH] = {0};
        snprintf(arg2, 9, "%zu", height);
        new_argv[0] = arg0;
        new_argv[1] = arg1;
        new_argv[2] = arg2;
        new_argv[3] = NULL;

        execve(view_name, new_argv, NULL);
        return ERROR;
    }
    return SUCCESS;
}
