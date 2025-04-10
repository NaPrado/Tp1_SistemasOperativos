#ifndef PARAMETERS_MASTER_H
#define PARAMETERS_MASTER_H

#include "structures.h"

int set_params(int argc, char * const argv[], Tparameters * params);

Tparameters get_default_params();

void print_inicial_state(Tparameters params);

#endif
