#ifndef DOODLE_LUA_H
#define DOODLE_LUA_H

#include <stdbool.h>
#include <stdio.h>

#include "doodle/doodle.h"

typedef enum {
    DOODLE_LERR_UNSET_GLOBAL,
    DOODLE_LERR_BAD_GLOBAL_TYPE,
    DOODLE_LERR_INIT_FAIL,
    DOODLE_LERR_LOAD_FAIL,
    DOODLE_LERR_RUN_FAIL,
    DOODLE_LERR_IMG_N_FAIL,
} doodle_lua_error_type;

typedef struct {
    doodle_lua_error_type et;
    char msg[];
} doodle_lua_error;

doodle_lua_error *doodle_lua_run_file(
    FILE *in, 
    doodle_image **img, 
    doodle_config *conf
);

#endif
