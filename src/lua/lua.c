#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lua_helpers.h"
#include "lua_point.h"
#include "doodle/doodle.h"

#define READER_BUF_SIZE 2048

typedef enum {
    RECTANGLE_DRAW,
    CIRCLE_DRAW,
    LINE_DRAW,
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

typedef struct {
    doodle_point p1;
    doodle_point p2;
    double thickness;
    doodle_color color;
} line_draw;

typedef struct draw {
    draw_type type;
    struct draw *next;
    union {
        rect_draw rect;
        circ_draw circle;
        line_draw line;
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

static doodle_lua_error *new_error(doodle_lua_error_type et, const char *msg) {
    doodle_lua_error *err = malloc(strlen(msg) + 1 + sizeof *err);
    err->et = et;
    strcpy(err->msg, msg);
    return err;
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

static doodle_lua_error *get_global_u32(
    lua_State *L, 
    const char *key, 
    uint32_t *n
) {
    static const char *required = "%s is a required field";
    static const char *bad_type = "%s must be a positive integer";

    lua_getglobal(L, key);
    if (lua_isnil(L, -1)) {
        char *msg = malloc(strlen(required) + strlen(key));
        sprintf(msg, required, key);
        doodle_lua_error *err = new_error(DOODLE_LERR_UNSET_GLOBAL, msg);
        free(msg);
        return err;
    }
    if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 1) {
        char *msg = malloc(strlen(bad_type) + strlen(key));
        sprintf(msg, bad_type, key);
        doodle_lua_error *err = new_error(DOODLE_LERR_BAD_GLOBAL_TYPE, msg);
        free(msg);
        return err;
    }

    *n = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return NULL;
}

static doodle_lua_error *get_global_userdata(
    lua_State *L, 
    const char *key, 
    const char *mt_name,
    void **data
) {
    static const char *required = "%s is a required field";
    static const char *bad_type = "%s must be a %s";

    lua_getglobal(L, key);
    if (lua_isnil(L, -1)) {
        char *msg = malloc(strlen(required) + strlen(key));
        sprintf(msg, required, key);
        doodle_lua_error *err = new_error(DOODLE_LERR_UNSET_GLOBAL, msg);
        free(msg);
        return err;
    }
    if (!lua_isuserdata(L, -1) || !has_metatable(L, mt_name)) {
        char *msg = malloc(strlen(bad_type) + strlen(key) + strlen(mt_name));
        sprintf(msg, bad_type, key, mt_name);
        doodle_lua_error *err = new_error(DOODLE_LERR_BAD_GLOBAL_TYPE, msg);
        free(msg);
        return err;
    }

    *data = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return NULL;
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

static int draw_line(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    doodle_point *p1;
    doodle_point *p2;
    double thickness;
    doodle_color *color;

    bool setp1 = geti_userdata(L, 1, "doodle.point", (void**)&p1);
    bool setp2 = geti_userdata(L, 2, "doodle.point", (void**)&p2);
    bool setthickness = geti_number(L, 3, &thickness);
    bool setcolor = geti_userdata(L, 4, "doodle.color", (void**)&color);

    setp1 = getf_userdata(L, "p1", "doodle.point", (void**)&p1) || setp1;
    setp2 = getf_userdata(L, "p2", "doodle.point", (void**)&p2) || setp2;
    setthickness = getf_number(L, "thickness", &thickness) || setthickness;
    setcolor = 
        getf_userdata(L, "color", "doodle.color", (void**)&color) || setcolor;

    struct { bool set; char *key; } checks[] = {
        {setp1, "p1"},
        {setp2, "p2"},
        {setthickness, "thickness"},
        {setcolor, "color"},
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof *checks; i++) {
        if (!checks[i].set) {
            lua_pushfstring(L, NOT_PROVIDED, "line", checks[i].key);
            lua_error(L);
        }
    }

    draw *d = malloc(sizeof *d);
    d->type = LINE_DRAW;
    d->next = NULL;
    d->params.line.p1 = *p1;
    d->params.line.p2 = *p2;
    d->params.line.thickness = thickness;
    d->params.line.color = *color;
    env_draw_queue_push(L, d);

    return 0;
}

static int set_global_functions(lua_State *L) {
    luaL_Reg global_functions[] = {
        {"point", create_point},
        {"rectangle", draw_rect},
        {"circle", draw_circle},
        {"line", draw_line},
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

doodle_lua_error *doodle_lua_run_file(
    FILE *in, 
    doodle_image **img, 
    doodle_config *conf
) {
    file_read_data f = { .in = in };

    draw_queue *queue;

    doodle_lua_error *err = NULL;

    lua_State *L = setup_state(&queue);
    if (L == NULL) {
        return new_error(DOODLE_LERR_INIT_FAIL, "lua setup failed");
    }

    if (lua_load(L, read_file, &f, "doodle script") != 0) {
        err = new_error( DOODLE_LERR_LOAD_FAIL, lua_tostring(L, -1));
        goto run_lua_close_exit;
    }

    if (lua_pcall(L, 0, 0, 0) != 0) {
        err = new_error( DOODLE_LERR_RUN_FAIL, lua_tostring(L, -1));
        goto run_lua_close_exit;
    }

    doodle_color *background;
    err = get_global_userdata(L, "background", "doodle.color", (void**)&background);
    if (err != NULL) goto run_lua_close_exit;

    err = get_global_u32(L, "width", &conf->width);
    if (err != NULL) goto run_lua_close_exit;

    err = get_global_u32(L, "height", &conf->height);
    if (err != NULL) goto run_lua_close_exit;

    conf->background = *background;

    *img = doodle_new(conf);
    if (*img == NULL) {
        lua_close(L);
        return new_error(
            DOODLE_LERR_IMG_N_FAIL,
            "image creation failed"
        );
    }

    for (draw *d = queue->root; d != NULL;) {
        switch (d->type) {
        case RECTANGLE_DRAW:
            doodle_draw_rect(
                *img, 
                d->params.rect.origin, 
                d->params.rect.width,
                d->params.rect.height,
                d->params.rect.color
            );
            break;
        case CIRCLE_DRAW:
            doodle_draw_circle(
                *img,
                d->params.circle.origin,
                d->params.circle.radius,
                d->params.circle.color
            );
            break;
        case LINE_DRAW:
            doodle_draw_line(
                *img,
                d->params.line.p1,
                d->params.line.p2,
                d->params.line.thickness,
                d->params.line.color
            );
            break;
        }
        draw *tmp = d;
        d = d->next;
        free(tmp);
    }
    queue->root = NULL;
    queue->tail = NULL;

run_lua_close_exit:
    lua_close(L);

    return err;
}

