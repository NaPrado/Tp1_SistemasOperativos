#include "shm.h"
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
        exit(EXIT_FAILURE);
    }
    fclose(file);

    // Generate the key
    key_t key = ftok(name, 1);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
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
	if (fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}
	//solo para crearla
	if (-1 == ftruncate(fd, size)) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
	p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return p;
}

#else

static void * create_shmem(const char * name, size_t size, mode_t mode, int * shm_id) {
    void * p;
    key_t key = generate_key(name);
    *shm_id = shmget(key, size, IPC_CREAT | IPC_EXCL | mode);
    if (*shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    p = shmat(*shm_id, NULL, 0);
    if (p == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
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

	if (fd == -1) {
		perror("get_game_state");
		exit(EXIT_FAILURE);
	}
	p = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(fd);
#else
    key_t key = ftok("/tmp/game_state", 1); // Generate a unique key
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    game_state_shmid = shmget(key, size, 0666); // Get shared memory
    if (game_state_shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    p = shmat(game_state_shmid, NULL, 0); // Attach shared memory
    if (p == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
#endif
	return (Tgame_state *) p;
}

Tgame_sync * get_game_sync() {
    void * p;
#ifdef _POSIX_VERSION
    int fd = shm_open("/game_sync", O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    p = mmap(NULL, sizeof(Tgame_sync), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    close(fd);
#else
    key_t key = ftok("/tmp/game_sync", 1);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    game_sync_shmid = shmget(key, sizeof(Tgame_sync), 0666);
    if (game_sync_shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    p = shmat(game_sync_shmid, NULL, 0);
    if (p == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
#endif
    return (Tgame_sync *) p;
}

void munmap_game_state(Tgame_state * ptr, size_t size) {
#ifdef _POSIX_VERSION
    munmap(ptr, size);
#else
    if (shmdt(ptr) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
#endif
}

void munmap_game_sync(Tgame_sync * ptr) {
#ifdef _POSIX_VERSION
    munmap(ptr, sizeof(Tgame_sync));
#else
    if (shmdt(ptr) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
#endif
}

void free_game_sync(Tgame_sync * ptr) {
#ifdef _POSIX_VERSION
    shm_unlink("/game_sync");
    munmap_game_sync(ptr);
#else
    if (shmctl(game_sync_shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
    munmap_game_sync(ptr);
#endif
}

void free_game_state(Tgame_state * ptr, size_t size) {
#ifdef _POSIX_VERSION
    shm_unlink("/game_state");
    munmap_game_state(ptr, size);
#else
    if (shmctl(game_state_shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
    munmap_game_state(ptr, size);
#endif
}


void exit_error(Tgame_state * game_ptr, Tgame_sync * sync_ptr, size_t size_game_state) {
    free_game_sync(sync_ptr);
	free_game_state(game_ptr, size_game_state);
    exit(EXIT_FAILURE);
}
