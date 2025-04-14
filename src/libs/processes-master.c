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

static int close_other_pipes(pipe_array pipes, int num_players, int current_player) {
    int i = STDERR_FILENO + 1;
    while (i < pipes[current_player][STDIN_FILENO]) {
        if (close(i) == -1) {
            perror("Error: close read end");
            return ERROR;
        }
        i++;
    }
    return SUCCESS;
}

int set_players_processes(Tparameters * params, Tgame_state * game_state, pipe_array pipes) {
    int pid = 0;
    for (int i = 0; i < params->amount_players; i++) {
        if (pipe(pipes[i]) == ERROR) {
            return ERROR;
        }
        if ((pid = fork()) < 0) {
            perror("Error: player fork failed");
            return ERROR;
        } else if (pid == 0) { // hijo
            if (close(pipes[i][READ_END]) == ERROR) {
                perror("Error: close read end");
                return ERROR;
            }
            if (dup2(pipes[i][WRITE_END], STDOUT_FILENO) == ERROR) {
                perror("Error: dup2 write end");
                return ERROR;
            }
            if (close(pipes[i][WRITE_END]) == -1) {
                perror("Error: close write end");
                return ERROR;
            }
            if (close_other_pipes(pipes, params->amount_players, i) == ERROR) {
                perror("Error: close other pipes");
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
            if (close(pipes[i][WRITE_END]) == ERROR) {
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
    return getpid();
}

void close_pending_pipes(pipe_array pipes, size_t amount_players) {
    for (size_t i = 0; i < amount_players; i++) {
        if (pipes[i][STDIN_FILENO] != ERROR) {
            close(pipes[i][STDIN_FILENO]);
        }
        if (pipes[i][STDOUT_FILENO] != ERROR) {
            close(pipes[i][STDOUT_FILENO]);
        }
    }
}

void wait_processes(Tgame_state * game_state, pid_t view_pid) {
    int status;
    waitpid(view_pid, &status, 0);
    if (WIFEXITED(status)) {
        printf("View exited (%d)\n", WEXITSTATUS(status));
    } else {
        printf("View process exited with an error\n");
    }

    for (size_t i = 0; i < game_state->amount_players; i++) {
        if (game_state->players[i].pid != ERROR) {
            waitpid(game_state->players[i].pid, &status, 0);
            if (WIFEXITED(status)) {
                printf("Player %s (%ld) exited (%d) with a score of %d / %d / %d\n", 
                       game_state->players[i].name_player,
                       i,
                       WEXITSTATUS(status),
                       game_state->players[i].points,
                       game_state->players[i].amount_valid_movements,
                       game_state->players[i].amount_invalid_movements);
            } else {
                printf("Player %s (%ld) exited with error\n", game_state->players[i].name_player, i);
            }
        }
    }
}
