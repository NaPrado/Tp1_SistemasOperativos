#include "shm.h"
#include "return-codes.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#ifndef _POSIX_VERSION
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>

static int game_state_shmid = -1;
static int game_sync_shmid = -1;

static key_t generate_key(const char *name) {
    // Ensure the file exists
    FILE *file = fopen(name, "w");
    if (!file) {
        perror("fopen");
        return ERROR;
    }
    fclose(file);

    // Generate the key
    key_t key = ftok(name, 1);
    if (key == ERROR) {
        perror("ftok");
        return ERROR;
    }
    return key;
}

#endif


#ifdef _POSIX_VERSION

static void * create_shmem(const char * name, size_t size, mode_t mode) {
    void * p;
	int fd;
	//shm_open(const char *name,int oflag, mode_t mode);
	fd = shm_open(name, O_RDWR | O_CREAT, mode/* 110110110 rwxrwxrwx*/);
	//shm_open es un open
	//mode solo para crear, sino se ignora
	if (fd == ERROR) {
		perror("shm_open");
		return NULL;
	}
	//solo para crearla
	if (ERROR == ftruncate(fd, size)) {
		perror("ftruncate");
		return NULL;
	}
	p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		return NULL;
	}
	close(fd);
	return p;
}

#else

static void * create_shmem(const char * name, size_t size, mode_t mode, int * shm_id) {
    void * p;
    key_t key = generate_key(name);
    *shm_id = shmget(key, size, IPC_CREAT | IPC_EXCL | mode);
    if (*shm_id == ERROR) {
        perror("shmget");
        return NULL;
    }
    p = shmat(*shm_id, NULL, 0);
    if (p == (void *)ERROR) {
        perror("shmat");
        return NULL;
    }
    memset(p, 0, size); // Initialize memory to zero
	return p;
}

#endif

Tgame_state * create_game_state(size_t size) {
#ifdef _POSIX_VERSION
    return (Tgame_state *) create_shmem("/game_state", size, 0644);
#else
    return (Tgame_state *) create_shmem("/tmp/game_state", size, 0644, &game_state_shmid);
#endif
}

Tgame_sync * create_game_sync() {
#ifdef _POSIX_VERSION
    return (Tgame_sync *) create_shmem("/game_sync", sizeof(Tgame_sync), 0666);
#else
    return (Tgame_sync *) create_shmem("/tmp/game_sync", sizeof(Tgame_sync), 0666, &game_sync_shmid);
#endif
}

Tgame_state * get_game_state(size_t size) {
    void * p;
#ifdef _POSIX_VERSION
	int fd;
	//shm_open(const char *name,int oflag, mode_t mode);
	fd = shm_open("/game_state", O_RDONLY, 0644/* 110100100 rwxrwxrwx*/);

	if (fd == ERROR) {
		perror("get_game_state");
		return NULL;
	}
	p = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		return NULL;
	}
	close(fd);
#else
    key_t key = ftok("/tmp/game_state", 1); // Generate a unique key
    if (key == ERROR) {
        perror("ftok");
        return NULL;
    }

    game_state_shmid = shmget(key, size, 0666); // Get shared memory
    if (game_state_shmid == ERROR) {
        perror("shmget");
        return NULL;
    }

    p = shmat(game_state_shmid, NULL, 0); // Attach shared memory
    if (p == (void *)ERROR) {
        perror("shmat");
        return NULL;
    }
#endif
	return (Tgame_state *) p;
}

Tgame_sync * get_game_sync() {
    void * p;
#ifdef _POSIX_VERSION
    int fd = shm_open("/game_sync", O_RDWR, 0666);
    if (fd == ERROR) {
        perror("shm_open");
        return NULL;
    }
    p = mmap(NULL, sizeof(Tgame_sync), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    close(fd);
#else
    key_t key = ftok("/tmp/game_sync", 1);
    if (key == ERROR) {
        perror("ftok");
        return NULL;
    }
    game_sync_shmid = shmget(key, sizeof(Tgame_sync), 0666);
    if (game_sync_shmid == ERROR) {
        perror("shmget");
        return NULL;
    }
    p = shmat(game_sync_shmid, NULL, 0);
    if (p == (void *)ERROR) {
        perror("shmat");
        return NULL;
    }
#endif
    return (Tgame_sync *) p;
}

int munmap_game_state(Tgame_state * ptr, size_t size) {
#ifdef _POSIX_VERSION
    if (munmap(ptr, size) == ERROR) {
        perror("munmap");
        return ERROR;
    }
#else
    if (shmdt(ptr) == ERROR) {
        perror("shmdt");
        return ERROR;
    }
#endif
    return SUCCESS;
}

int munmap_game_sync(Tgame_sync * ptr) {
#ifdef _POSIX_VERSION
    if (munmap(ptr, sizeof(Tgame_sync)) == ERROR) {
        perror("munmap");
        return ERROR;
    }
#else
    if (shmdt(ptr) == ERROR) {
        perror("shmdt");
    }
#endif
    return SUCCESS;
}

void free_game_sync(Tgame_sync * ptr) {
#ifdef _POSIX_VERSION
    shm_unlink("/game_sync");
    munmap_game_sync(ptr);
#else
    if (shmctl(game_sync_shmid, IPC_RMID, NULL) == ERROR) {
        perror("shmctl IPC_RMID");
    }
    munmap_game_sync(ptr);
#endif
}

void free_game_state(Tgame_state * ptr, size_t size) {
#ifdef _POSIX_VERSION
    shm_unlink("/game_state");
    munmap_game_state(ptr, size);
#else
    if (shmctl(game_state_shmid, IPC_RMID, NULL) == ERROR) {
        perror("shmctl IPC_RMID");
    }
    munmap_game_state(ptr, size);
#endif
}


void exit_error(Tgame_state * game_ptr, Tgame_sync * sync_ptr, size_t size_game_state) {
    free_game_sync(sync_ptr);
	free_game_state(game_ptr, size_game_state);
    exit(EXIT_FAILURE);
}
