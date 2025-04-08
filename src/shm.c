#include "../include/shm.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


void * create_shmem(const char * name, size_t size, mode_t mode) {
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
	void * p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return p;
}
game_status * create_game_state(size_t size) {
	return (game_status *) create_shmem("/game_state", sizeof(game_status) + (sizeof(int) * size), 0644);
}
semaphores_status * create_game_sync() {
	return (semaphores_status *) create_shmem("/game_sync", sizeof(semaphores_status), 0666);
}

game_status * get_game_state(size_t size) {
	int fd;
	//shm_open(const char *name,int oflag, mode_t mode);
	fd = shm_open("/game_state", O_RDONLY, 0644/* 110100100 rwxrwxrwx*/);
	//shm_open es un open
	//mode solo para crear, sino se ignora
	if (fd == -1) {
		perror("get_game_state");
		exit(EXIT_FAILURE);
	}
	void * p = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return (game_status *) p;
}

semaphores_status * get_game_sync() {
	int fd;
	//shm_open(const char *name,int oflag, mode_t mode);
	fd = shm_open("/game_sync", O_RDWR, 0666/* 110110110 rwxrwxrwx*/);
	//shm_open es un open
	//mode solo para crear, sino se ignora
	if (fd == -1) {
		perror("get_game_sync");
		exit(EXIT_FAILURE);
	}
	void * p = mmap(NULL, sizeof(semaphores_status), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return (semaphores_status *) p;
}
void munmap_game_state(game_status * ptr, size_t size) {
	munmap(ptr, size);
}
void munmap_game_sync(semaphores_status * ptr) {
	munmap((void*)ptr, sizeof(semaphores_status));
}

void free_game_sync(semaphores_status * ptr) {
	shm_unlink("/game_sync");
	munmap_game_sync(ptr);
}

void free_game_state(game_status * ptr, size_t size) {
	shm_unlink("/game_state");
	munmap_game_state(ptr, size);
}
