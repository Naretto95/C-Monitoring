/*!
\file common.c
\brief Implementation of the small shared utilities declared in common.h.
*/

#include <stdio.h>
#include <stdlib.h>
#include "common.h"

void die(const char *error_message) {
    perror(error_message);
    exit(EXIT_FAILURE);
}
