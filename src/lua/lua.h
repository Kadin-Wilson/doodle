#ifndef DOODLE_LUA_H
#define DOODLE_LUA_H

#include <stdbool.h>
#include <stdio.h>

#include "doodle/doodle.h"

#define MAX_FILE_BYTES 10000

doodle_image *run_file(FILE *in);

#endif
