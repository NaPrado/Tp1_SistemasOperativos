#ifndef SHM_H
#define SHM_H
#include <stdlib.h>
void * create_SHM(const char * name,size_t size);
void * get_open_SHM(const char * name,size_t size);
#endif