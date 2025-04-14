#include "sync-lib.h"
#include "structures.h"
#include "return-codes.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef _POSIX_VERSION

#include <sys/sem.h>
#include <errno.h>

enum semaphores {
    SHOW_NEEDED,
    SHOW_DONE,
    MASTER_MUTEX,
    GAME_STATE_MUTEX,
    PLAYER_READ_COUNT_MUTEX
};

enum sem_ops {
    SEM_OP_WAIT = -1,
    SEM_OP_POST = 1
};

#define FLAG_NULL 0
#define SEM_PATH   "/tmp/game_sync"
#define SEM_PROJID 'S'
#define NSEMS      5
static int sem_id = -1;

int init_systemv_semaphores(void) {
    key_t key = ftok(SEM_PATH, SEM_PROJID);
    if (key == ERROR) {
        perror("ftok"); return ERROR;
    }

    sem_id = semget(key, NSEMS, 0);  // 0 = sin crear, sólo abrir
    if (sem_id == ERROR) {
        perror("semget"); return ERROR;
    }

    return 0;
}

static int semop_retry(int semid, struct sembuf *sops, size_t nsops) {
    int r;
    do { 
        r = semop(semid, sops, nsops);
    } while (r == ERROR && errno == EINTR);
    return r;
}

#endif



int set_init_semaphores(Tgame_sync * game_sync) {

#ifdef _POSIX_VERSION

    sem_init(&game_sync->show_needed, 1, 0);
    sem_init(&game_sync->show_done, 1, 0);
    sem_init(&game_sync->master_mutex, 1, 1);
    sem_init(&game_sync->game_state_mutex, 1, 1);
    sem_init(&game_sync->player_read_count_mutex, 1, 1);

#else 
    key_t key = ftok(SEM_PATH, SEM_PROJID);
    if (key == ERROR) {
        perror("ftok");
        return ERROR; 
    }

    sem_id = semget(key, NSEMS, IPC_CREAT | 0666);
    if (sem_id == ERROR) {
        perror("semget failed");
        return ERROR;
    }

    // Initialize semaphores
    if (semctl(sem_id, SHOW_NEEDED, SETVAL, 0) == ERROR) {
        perror("semctl SHOW_NEEDED failed");
        return ERROR;
    }
    if (semctl(sem_id, SHOW_DONE, SETVAL, 0) == ERROR) {
        perror("semctl SHOW_DONE failed");
        return ERROR;
    }
    if (semctl(sem_id, MASTER_MUTEX, SETVAL, 1) == ERROR) {
        perror("semctl MASTER_MUTEX failed");
        return ERROR;
    }
    if (semctl(sem_id, GAME_STATE_MUTEX, SETVAL, 1) == ERROR) {
        perror("semctl GAME_STATE_MUTEX failed");
        return ERROR;
    }
    if (semctl(sem_id, PLAYER_READ_COUNT_MUTEX, SETVAL, 1) == ERROR) {
        perror("semctl PLAYER_READ_COUNT_MUTEX failed");
        return ERROR;
    }

#endif

    game_sync->player_reading_status = 0;
    return SUCCESS;
}

int set_master_writing(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_wait(&game_sync->master_mutex);
    sem_wait(&game_sync->game_state_mutex);
    sem_post(&game_sync->master_mutex);
#else
    struct sembuf op1 = {MASTER_MUTEX, SEM_OP_WAIT, FLAG_NULL};
    if (semop_retry(sem_id, &op1, 1) == ERROR) {
        perror("semop set_master_writing (MASTER_MUTEX WAIT) failed");
        return ERROR;
    }

    struct sembuf op2 = {GAME_STATE_MUTEX, SEM_OP_WAIT, FLAG_NULL};
    if (semop_retry(sem_id, &op2, 1) == ERROR) {
        perror("semop set_master_writing (GAME_STATE_MUTEX WAIT) failed");
        return ERROR;
    }

    struct sembuf op3 = {MASTER_MUTEX, SEM_OP_POST, IPC_NOWAIT};
    if (semop_retry(sem_id, &op3, 1) == ERROR) {
        perror("semop set_master_writing (MASTER_MUTEX POST) failed");
        return ERROR;
    }
#endif
    return SUCCESS;
}

int unset_master_writing(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_post(&game_sync->game_state_mutex);
#else
    struct sembuf ops[1] = {
        {GAME_STATE_MUTEX, SEM_OP_POST, IPC_NOWAIT}
    };
    if (semop_retry(sem_id, ops, 1) == ERROR) {
        perror("semop unset_master_writing failed");
        return ERROR;
    }
#endif
    return SUCCESS;
}

int master_signal_print(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_post(&game_sync->show_needed);
    sem_wait(&game_sync->show_done);
#else
    struct sembuf op1 = {SHOW_NEEDED, SEM_OP_POST, IPC_NOWAIT};
    if (semop_retry(sem_id, &op1, 1) == ERROR) {
        perror("semop master_signal_print failed");
        return ERROR;
    }
    struct sembuf op2 = {SHOW_DONE, SEM_OP_WAIT, FLAG_NULL};
    if (semop_retry(sem_id, &op2, 1) == ERROR) {
        perror("semop master_signal_print failed");
        return ERROR;
    }
#endif
    return SUCCESS;
}

int set_player_reading(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_wait(&game_sync->master_mutex);
    sem_post(&game_sync->master_mutex);
    sem_wait(&game_sync->player_read_count_mutex); // espero a modificar variable
    if (game_sync->player_reading_status++ == 0) 
        sem_wait(&game_sync->game_state_mutex); // espero a que writer libere
    sem_post(&game_sync->player_read_count_mutex); // dejo modificar variable
#else
    struct sembuf op1 = {MASTER_MUTEX, SEM_OP_WAIT, FLAG_NULL};
    if (semop_retry(sem_id, &op1, 1) == ERROR) {
        perror("semop set_player_reading (MASTER_MUTEX WAIT) failed");
        return ERROR;
    }

    struct sembuf op2 = {PLAYER_READ_COUNT_MUTEX, SEM_OP_WAIT, FLAG_NULL};
    if (semop_retry(sem_id, &op2, 1) == ERROR) {
        perror("semop set_player_reading (PLAYER_READ_COUNT_MUTEX WAIT) failed");
        return ERROR;
    }

    struct sembuf op3 = {MASTER_MUTEX, SEM_OP_POST, IPC_NOWAIT};
    if (semop_retry(sem_id, &op3, 1) == ERROR) {
        perror("semop set_player_reading (MASTER_MUTEX POST) failed");
        return ERROR;
    }

    if (game_sync->player_reading_status++ == 0) {
        struct sembuf ops2[1] = {
            {GAME_STATE_MUTEX, SEM_OP_WAIT, FLAG_NULL}
        };
        if (semop_retry(sem_id, ops2, 1) == ERROR) {
            perror("semop set_player_reading failed");
            return ERROR;
        }
    }

    struct sembuf ops[1] = {
        {PLAYER_READ_COUNT_MUTEX, SEM_OP_POST, IPC_NOWAIT}
    };
    if (semop_retry(sem_id, ops, 1) == ERROR) {
        perror("semop set_player_reading failed");
        return ERROR;
    }
#endif
    return SUCCESS;
}

int unset_player_reading(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_wait(&game_sync->player_read_count_mutex); // espero a modificar variable
    if (game_sync->player_reading_status-- == 1)
        sem_post(&game_sync->game_state_mutex); // dejo al writer
    sem_post(&game_sync->player_read_count_mutex); // dejo modificar variable
#else
    struct sembuf op1 = {PLAYER_READ_COUNT_MUTEX, SEM_OP_WAIT, FLAG_NULL};
    if (semop_retry(sem_id, &op1, 1) == ERROR) {
        perror("semop unset_player_reading (PLAYER_READ_COUNT_MUTEX WAIT) failed");
        return ERROR;
    }

    struct sembuf op2 = {PLAYER_READ_COUNT_MUTEX, SEM_OP_POST, IPC_NOWAIT};
    if (semop_retry(sem_id, &op2, 1) == ERROR) {
        perror("semop unset_player_reading (PLAYER_READ_COUNT_MUTEX POST) failed");
        return ERROR;
    }

    if (game_sync->player_reading_status-- == 1) {
        struct sembuf ops2[1] = {
            {GAME_STATE_MUTEX, SEM_OP_POST, IPC_NOWAIT}
        };
        if (semop_retry(sem_id, ops2, 1) == ERROR) {
            perror("semop unset_player_reading failed");
            return ERROR;
        }
    }
#endif
    return SUCCESS;
}

int wait_view(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_wait(&game_sync->show_needed);
#else
    struct sembuf ops[1] = {
        {SHOW_NEEDED, SEM_OP_WAIT, FLAG_NULL}
    };
    if (semop_retry(sem_id, ops, 1) == ERROR) {
        perror("semop wait_view failed");
        return ERROR;
    }
#endif
    return SUCCESS;
}

int signal_view(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_post(&game_sync->show_done);
#else
    struct sembuf ops[1] = {
        {SHOW_DONE, SEM_OP_POST, IPC_NOWAIT}
    };
    if (semop_retry(sem_id, ops, 1) == ERROR) {
        perror("semop signal_view failed");
        return ERROR;
    }
#endif
    return SUCCESS;
}

int destroy_semaphores(Tgame_sync * game_sync) {
#ifdef _POSIX_VERSION
    sem_destroy(&game_sync->show_needed);
    sem_destroy(&game_sync->show_done);
    sem_destroy(&game_sync->master_mutex);
    sem_destroy(&game_sync->game_state_mutex);
    sem_destroy(&game_sync->player_read_count_mutex);
#else
    if (semctl(sem_id, 0, IPC_RMID) == ERROR) {
        perror("semctl failed");
        return ERROR;
    }
#endif
    return SUCCESS;
}
