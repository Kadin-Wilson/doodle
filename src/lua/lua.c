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

static const char *not_provided = "failed to provide a value for %s";

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

static void dumpstack(lua_State *L) {
    int top=lua_gettop(L);
    for (int i=1; i <= top; i++) {
        printf("%d\t%s\t", i, luaL_typename(L,i));
        switch (lua_type(L, i)) {
        case LUA_TNUMBER:
            printf("%g\n",lua_tonumber(L,i));
            break;
        case LUA_TSTRING:
            printf("%s\n",lua_tostring(L,i));
            break;
        case LUA_TBOOLEAN:
            printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
            break;
        case LUA_TNIL:
            printf("%s\n", "nil");
            break;
        default:
            printf("%p\n",lua_topointer(L,i));
            break;
        }
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

static bool has_metatable(lua_State *L, const char *name) {
    if (!lua_getmetatable(L, -1)) {
        return false;
    }
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

static int create_point(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    uint32_t x, y;
    bool setx = false;
    bool sety = false;

    // coords from array
    lua_rawgeti(L, 1, 1);
    if (lua_isnumber(L, -1)) {
        x = lua_tonumber(L, -1);
        setx = true;
    }
    lua_rawgeti(L, 1, 2);
    if (lua_isnumber(L, -1)) {
        y = lua_tonumber(L, -1);
        sety = true;
    }

    // coords from table keys
    lua_getfield(L, 1, "x");
    if (lua_isnumber(L, -1)) {
        x = lua_tonumber(L, -1);
        setx = true;
    }
    lua_getfield(L, 1, "y");
    if (lua_isnumber(L, -1)) {
        y = lua_tonumber(L, -1);
        sety = true;
    }

    lua_pop(L, 4);

    if (!setx) {
        lua_pushfstring(L, not_provided, "x");
        lua_error(L);
    }
    if (!sety) {
        lua_pushfstring(L, not_provided, "y");
        lua_error(L);
    }

    doodle_point *p = lua_newuserdata(L, sizeof *p);
    p->x = x;
    p->y = y;
    luaL_newmetatable(L, "doodle.point");
    lua_setmetatable(L, -2);

    return 1;
}

static int draw_rect(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    doodle_point origin;
    uint32_t width, height;
    doodle_color color;
    bool setorigin = false;
    bool setwidth = false;
    bool setheight = false;
    bool setcolor = false;

    lua_rawgeti(L, 1, 1);
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.point")) {
        origin = *(doodle_point*)lua_touserdata(L, -1);
        setorigin = true;
    }
    lua_rawgeti(L, 1, 2);
    if (lua_isnumber(L, -1)) {
        width = lua_tonumber(L, -1);
        setwidth = true;
    }
    lua_rawgeti(L, 1, 3);
    if (lua_isnumber(L, -1)) {
        height = lua_tonumber(L, -1);
        setheight = true;
    }
    lua_rawgeti(L, 1, 4);
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.color")) {
        color = *(doodle_color*)lua_touserdata(L, -1);
        setcolor = true;
    }
    lua_pop(L, 4);

    lua_getfield(L, 1, "origin");
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.point")) {
        origin = *(doodle_point*)lua_touserdata(L, -1);
        setorigin = true;
    }
    lua_getfield(L, 1, "width");
    if (lua_isnumber(L, -1)) {
        width = lua_tonumber(L, -1);
        setwidth = true;
    }
    lua_getfield(L, 1, "height");
    if (lua_isnumber(L, -1)) {
        height = lua_tonumber(L, -1);
        setheight = true;
    }
    lua_getfield(L, 1, "color");
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.color")) {
        color = *(doodle_color*)lua_touserdata(L, -1);
        setcolor = true;
    }
    lua_pop(L, 4);

    struct { bool set; char *key; } checks[] = {
        {setorigin, "origin"},
        {setwidth, "width"},
        {setheight, "height"},
        {setcolor, "color"},
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof *checks; i++) {
        if (!checks[i].set) {
            lua_pushfstring(L, not_provided, checks[i].key);
            lua_error(L);
        }
    }

    draw *d = malloc(sizeof *d);
    d->type = RECTANGLE_DRAW;
    d->next = NULL;
    d->params.rect.origin = origin;
    d->params.rect.width = width;
    d->params.rect.height = height;
    d->params.rect.color = color;

    lua_getfield(L, LUA_ENVIRONINDEX, "draw_queue");
    draw_queue *queue = lua_touserdata(L, -1);

    if (queue->root == NULL) {
        queue->root = d;
        queue->tail = d;
    } else {
        queue->tail->next = d;
        queue->tail = d;
    }

    return 0;
}

static int draw_circle(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    doodle_point origin;
    uint32_t radius;
    doodle_color color;
    bool setorigin = false;
    bool setradius = false;
    bool setcolor = false;

    lua_rawgeti(L, 1, 1);
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.point")) {
        origin = *(doodle_point*)lua_touserdata(L, -1);
        setorigin = true;
    }
    lua_rawgeti(L, 1, 2);
    if (lua_isnumber(L, -1)) {
        radius = lua_tonumber(L, -1);
        setradius = true;
    }
    lua_rawgeti(L, 1, 3);
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.color")) {
        color = *(doodle_color*)lua_touserdata(L, -1);
        setcolor = true;
    }
    lua_pop(L, 3);

    lua_getfield(L, 1, "origin");
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.point")) {
        origin = *(doodle_point*)lua_touserdata(L, -1);
        setorigin = true;
    }
    lua_getfield(L, 1, "radius");
    if (lua_isnumber(L, -1)) {
        radius = lua_tonumber(L, -1);
        setradius = true;
    }
    lua_getfield(L, 1, "color");
    if (lua_isuserdata(L, -1) && has_metatable(L, "doodle.color")) {
        color = *(doodle_color*)lua_touserdata(L, -1);
        setcolor = true;
    }
    lua_pop(L, 3);

    struct { bool set; char *key; } checks[] = {
        {setorigin, "origin"},
        {setradius, "radius"},
        {setcolor, "color"},
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof *checks; i++) {
        if (!checks[i].set) {
            lua_pushfstring(L, not_provided, checks[i].key);
            lua_error(L);
        }
    }

    draw *d = malloc(sizeof *d);
    d->type = CIRCLE_DRAW;
    d->next = NULL;
    d->params.circle.origin = origin;
    d->params.circle.radius = radius;
    d->params.circle.color = color;

    lua_getfield(L, LUA_ENVIRONINDEX, "draw_queue");
    draw_queue *queue = lua_touserdata(L, -1);

    if (queue->root == NULL) {
        queue->root = d;
        queue->tail = d;
    } else {
        queue->tail->next = d;
        queue->tail = d;
    }

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

