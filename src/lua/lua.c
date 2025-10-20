#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lua.h"
#include "lua_helpers.h"
#include "lua_point.h"
#include "doodle/doodle.h"

#define READER_BUF_SIZE 2048

typedef enum {
    RECTANGLE_DRAW,
    CIRCLE_DRAW,
} draw_type;

typedef struct {
    doodle_point origin;
    uint32_t width;
    uint32_t height;
    doodle_color color;
} rect_draw;

typedef struct {
    doodle_point origin;
    uint32_t radius;
    doodle_color color;
} circ_draw;

typedef struct draw {
    draw_type type;
    struct draw *next;
    union {
        rect_draw rect;
        circ_draw circle;
    } params;
} draw;

typedef struct {
    draw *root;
    draw *tail;
} draw_queue;

typedef struct {
    FILE *in;
    char buf[READER_BUF_SIZE];
} file_read_data;

static void env_draw_queue_push(lua_State *L, draw *d) {
    lua_getfield(L, LUA_ENVIRONINDEX, "draw_queue");
    draw_queue *queue = lua_touserdata(L, -1);

    if (queue->root == NULL) {
        queue->root = d;
        queue->tail = d;
    } else {
        queue->tail->next = d;
        queue->tail = d;
    }
}

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

static double get_global_num(lua_State *L, const char *key) {
    lua_getglobal(L, key);
    if (!lua_isnumber(L, -1)) {
        error_required_field(L, key, lua_type(L, -1));
    }
    double n = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return n;
}

static int draw_rect(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    doodle_point *origin;
    double width, height;
    doodle_color *color;

    bool setorigin = geti_userdata(L, 1, "doodle.point", (void**)&origin);
    bool setwidth = geti_number(L, 2, &width);
    bool setheight = geti_number(L, 3, &height);
    bool setcolor = geti_userdata(L, 4, "doodle.color", (void**)&color);

    setorigin = 
        getf_userdata(L, "origin", "doodle.point", (void**)&origin) || setorigin;
    setwidth = getf_number(L, "width", &width) || setwidth;
    setheight = getf_number(L, "height", &height) || setheight;
    setcolor = 
        getf_userdata(L, "color", "doodle.color", (void**)&color) || setcolor;

    struct { bool set; char *key; } checks[] = {
        {setorigin, "origin"},
        {setwidth, "width"},
        {setheight, "height"},
        {setcolor, "color"},
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof *checks; i++) {
        if (!checks[i].set) {
            lua_pushfstring(L, NOT_PROVIDED, "rectangle", checks[i].key);
            lua_error(L);
        }
    }

    draw *d = malloc(sizeof *d);
    d->type = RECTANGLE_DRAW;
    d->next = NULL;
    d->params.rect.origin = *origin;
    d->params.rect.width = width;
    d->params.rect.height = height;
    d->params.rect.color = *color;
    env_draw_queue_push(L, d);

    return 0;
}

static int draw_circle(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    doodle_point *origin;
    double radius;
    doodle_color *color;

    bool setorigin = geti_userdata(L, 1, "doodle.point", (void**)&origin);
    bool setradius = geti_number(L, 2, &radius);
    bool setcolor = geti_userdata(L, 3, "doodle.color", (void**)&color);

    setorigin = 
        getf_userdata(L, "origin", "doodle.point", (void**)&origin) || setorigin;
    setradius = getf_number(L, "radius", &radius) || setradius;
    setcolor = 
        getf_userdata(L, "color", "doodle.color", (void**)&color) || setcolor;

    struct { bool set; char *key; } checks[] = {
        {setorigin, "origin"},
        {setradius, "radius"},
        {setcolor, "color"},
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof *checks; i++) {
        if (!checks[i].set) {
            lua_pushfstring(L, NOT_PROVIDED, "circle", checks[i].key);
            lua_error(L);
        }
    }

    draw *d = malloc(sizeof *d);
    d->type = CIRCLE_DRAW;
    d->next = NULL;
    d->params.circle.origin = *origin;
    d->params.circle.radius = radius;
    d->params.circle.color = *color;
    env_draw_queue_push(L, d);

    return 0;
}

static int set_global_functions(lua_State *L) {
    luaL_Reg global_functions[] = {
        {"point", create_point},
        {"rectangle", draw_rect},
        {"circle", draw_circle},
        {NULL, NULL}
    };

    lua_newtable(L);
    draw_queue *queue = lua_newuserdata(L, sizeof *queue);
    queue->root = NULL;
    queue->tail = NULL;
    lua_setfield(L, -2, "draw_queue");
    lua_replace(L, LUA_ENVIRONINDEX);

    for (size_t i = 0; global_functions[i].name != NULL; i++) {
        lua_pushcfunction(L, global_functions[i].func);
        lua_setglobal(L, global_functions[i].name);
    }

    lua_pushlightuserdata(L, queue);

    return 1;
}

static lua_State *setup_state(draw_queue **queue) {
    lua_State *L = luaL_newstate();
    if (L == NULL) {
        return NULL;
    }

    luaopen_math(L);
    luaopen_base(L);

    struct {
        const char *name;
        doodle_color color;
    } global_colors[] = {
        {"RED", {255, 0, 0, 0}},
        {"GREEN", {0, 255, 0, 0}},
        {"BLUE", {0, 0, 255, 0}},
        {"WHITE", {255, 255, 255, 0}},
        {"BLACK", {0, 0, 0, 0}},
        {NULL, {}}
    };

    for (size_t i = 0; global_colors[i].name != NULL; i++) {
        doodle_color *color = lua_newuserdata(L, sizeof(*color));
        *color = global_colors[i].color;

        luaL_newmetatable(L, "doodle.color");
        lua_setmetatable(L, -2);

        lua_setglobal(L, global_colors[i].name);
    }

    lua_getglobal(L, "BLACK");
    lua_setglobal(L, "background");

    lua_pushcfunction(L, set_global_functions);
    lua_call(L, 0, 1);

    *queue = lua_touserdata(L, -1);
    lua_pop(L, 1);

    return L;
}

bool run_file(FILE *in) {
    file_read_data f = { .in = in };

    draw_queue *queue;

    lua_State *L = setup_state(&queue);
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
    if (!lua_isuserdata(L, -1) || !has_metatable(L, "doodle.color")) {
        fputs("background must be set to a color\n", stderr);
        exit(EXIT_FAILURE);
    }
    doodle_color *color = lua_touserdata(L, -1);

    doodle_image *img = doodle_new(width, height, *color);

    for (draw *d = queue->root; d != NULL;) {
        switch (d->type) {
        case RECTANGLE_DRAW:
            doodle_draw_rect(
                img, 
                d->params.rect.origin, 
                d->params.rect.width,
                d->params.rect.height,
                d->params.rect.color
            );
            break;
        case CIRCLE_DRAW:
            doodle_draw_circle(
                img,
                d->params.circle.origin,
                d->params.circle.radius,
                d->params.circle.color
            );
            break;
        }
        draw *tmp = d;
        d = d->next;
        free(tmp);
    }
    queue->root = NULL;
    queue->tail = NULL;

    doodle_export_ppm(img, stdout);

    free(img);
    lua_close(L);

    return true;
}

