#include "../include/shm.h"


void * createSHM(const char * name, size_t size){
	int fd;
	//shm_open(const char *name,int oflag, mode_t mode);
	fd = shm_open(name, O_RDWR | O_CREAT, 0666/* 110110110 rwxrwxrwx*/);
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
	void * p = mmap(NULL, size, PROT_WRITE | PROT_READ,MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return p;
}

void * getOpenSHM(const char * name, size_t size){
	int fd;
	//shm_open(const char *name,int oflag, mode_t mode);
	fd = shm_open(name, O_RDONLY, 0644/* 110100100 rwxrwxrwx*/);
	//shm_open es un open
	//mode solo para crear, sino se ignora
	if (fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}
	void * p = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(fd);
	return p;
}


