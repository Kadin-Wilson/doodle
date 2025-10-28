#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <stdint.h>

#include "doodle/doodle.h"
#include "lua/lua_helpers.h"

static void push_metatable(lua_State *L) {
    static const char *meta_name = "doodle.color";

    if (!luaL_newmetatable(L, meta_name)) {
        return;
    }

    luaL_Reg funcs[] = {
        { NULL, NULL }
    };
    luaL_register(L, NULL, funcs);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return;
};

static void push_color_userdata(lua_State *L, doodle_color c) {
    doodle_color *udp = lua_newuserdata(L, sizeof *udp);
    udp->r = c.r;
    udp->g = c.g;
    udp->b = c.b;
    udp->a = c.a;
    push_metatable(L);
    lua_setmetatable(L, -2);
}

int create_color(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    doodle_color c = {.r = 0, .g = 0, .b = 0, .a = 0};

    geti_u8(L, 1, &c.r);
    geti_u8(L, 2, &c.g);
    geti_u8(L, 3, &c.b);
    geti_u8(L, 4, &c.a);

    getf_u8(L, "r", &c.r);
    getf_u8(L, "g", &c.g);
    getf_u8(L, "b", &c.b);
    getf_u8(L, "a", &c.a);

    push_color_userdata(L, c);

    return 1;
}

void set_global_colors(lua_State *L) {
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
        push_color_userdata(L, global_colors[i].color);
        lua_setglobal(L, global_colors[i].name);
    }
}
