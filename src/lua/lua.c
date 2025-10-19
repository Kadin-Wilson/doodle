#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lua.h"
#include "doodle/doodle.h"

#define READER_BUF_SIZE 2048

typedef struct {
    FILE *in;
    char buf[READER_BUF_SIZE];
} file_read_data;


static const char *read_file(lua_State *L, void *data, size_t *size) {
    file_read_data *f = data;
    if (feof(f->in) || ferror(f->in)) {
        *size = 0;
        return NULL;
    }

    size_t rc = fread(f->buf, 1, READER_BUF_SIZE, f->in);
    if (rc == 0) {
        *size = 0;
        return NULL;
    }

    *size = rc;
    return f->buf;
}

static void error_required_field(lua_State *L, const char *key, int type) {
    if (type == LUA_TNIL) {
        fprintf(stderr, "error: %s needs to be set\n", key);
    } else {
        fprintf(stderr, "error: %s cannot be a %s\n", 
                        key, lua_typename(L, type));
    }
    exit(EXIT_FAILURE);
}

static bool has_metatable(lua_State *L, const char *name) {
    lua_getmetatable(L, -1);
    luaL_getmetatable(L, name);
    bool has = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return has;
}

static double get_global_num(lua_State *L, const char *key) {
    lua_getglobal(L, key);
    if (!lua_isnumber(L, -1)) {
        error_required_field(L, key, lua_type(L, -1));
    }
    double n = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return n;
}

static lua_State *setup_state() {
    struct {
        char *name;
        doodle_color color;
    } global_colors[] = {
        {"RED", {255, 0, 0, 0}},
        {"GREEN", {0, 255, 0, 0}},
        {"BLUE", {0, 0, 255, 0}},
        {"WHITE", {255, 255, 255, 0}},
        {"BLACK", {0, 0, 0, 0}},
        {NULL, {}}
    };

    lua_State *L = luaL_newstate();
    if (L == NULL) {
        return NULL;
    }

    luaopen_math(L);

    for (size_t i = 0; global_colors[i].name != NULL; i++) {
        doodle_color *color = lua_newuserdata(L, sizeof(*color));
        *color = global_colors[i].color;

        luaL_newmetatable(L, "doodle.color");
        lua_setmetatable(L, -2);

        lua_setglobal(L, global_colors[i].name);
    }

    lua_getglobal(L, "BLACK");
    lua_setglobal(L, "background");

    return L;
}

bool run_file(FILE *in) {
    file_read_data f = { .in = in };

    lua_State *L = setup_state();
    if (L == NULL) {
        fputs("failed to intialize lua", stderr);
        return false;
    }

    if (lua_load(L, read_file, &f, "doodle input") != 0) {
        fprintf(stderr, "Error loading script: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "Error running script: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return false;
    }

    uint32_t width = get_global_num(L, "width");
    uint32_t height = get_global_num(L, "height");

    lua_getglobal(L, "background");
    if (!has_metatable(L, "doodle.color")) {
        fputs("background must be set to a color\n", stderr);
        exit(EXIT_FAILURE);
    }
    doodle_color *color = lua_touserdata(L, -1);

    doodle_image *img = doodle_new(width, height, *color);
    doodle_export_ppm(img, stdout);

    //free(img);
    lua_close(L);

    return true;
}

