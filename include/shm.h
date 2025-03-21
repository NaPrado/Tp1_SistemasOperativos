#ifndef SHM_H
#define SHM_H
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
void * createSHM(const char * name,size_t size, int flags,int prot);
#endif