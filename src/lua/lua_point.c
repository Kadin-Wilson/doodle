#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <stdbool.h>
#include <stdint.h>

#include "doodle/point.h"
#include "lua_helpers.h"
#include "lua_point.h"

static void push_metatable(lua_State *L) {
    static const char *meta_name = "doodle.point";

    if (!luaL_newmetatable(L, meta_name)) {
        return;
    }

    luaL_Reg funcs[] = {
        { "__add", add_points },
        { "__sub", subtract_points },
        { "__mul", scale_point },
        { "add", add_points },
        { "subtract", subtract_points },
        { "scale", scale_point },
        { "polar_offset", polar_offset_point },
        { NULL, NULL }
    };
    luaL_register(L, NULL, funcs);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return;
};

static void push_point_userdata(lua_State *L, doodle_point p) {
    doodle_point *udp = lua_newuserdata(L, sizeof *udp);
    udp->x = p.x;
    udp->y = p.y;
    push_metatable(L);
    lua_setmetatable(L, -2);
}

int create_point(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    double x, y;
    bool setx = false;
    bool sety = false;

    // coords from array
    setx = geti_number(L, 1, &x);
    sety = geti_number(L, 2, &y);

    // coords from table keys
    setx = getf_number(L, "x", &x) || setx;
    sety = getf_number(L, "y", &y) || sety;

    if (!setx) {
        lua_pushfstring(L, NOT_PROVIDED, "point", "x");
        lua_error(L);
    }
    if (!sety) {
        lua_pushfstring(L, NOT_PROVIDED, "point", "y");
        lua_error(L);
    }

    push_point_userdata(L, (doodle_point){ .x = x, .y = y });

    return 1;
}

int add_points(lua_State *L) {
    doodle_point *p1 = luaL_checkudata(L, 1, "doodle.point");
    doodle_point *p2 = luaL_checkudata(L, 2, "doodle.point");

    push_point_userdata(L, doodle_point_add(*p1, *p2));

    return 1;
}

int subtract_points(lua_State *L) {
    doodle_point *p1 = luaL_checkudata(L, 1, "doodle.point");
    doodle_point *p2 = luaL_checkudata(L, 2, "doodle.point");

    push_point_userdata(L, doodle_point_subtract(*p1, *p2));

    return 1;
}

int scale_point(lua_State *L) {
    doodle_point *p = luaL_checkudata(L, 1, "doodle.point");
    double factor = luaL_checknumber(L, 2);

    push_point_userdata(L, doodle_point_scale(*p, factor));

    return 1;
}

int polar_offset_point(lua_State *L) {
    doodle_point *p = luaL_checkudata(L, 1, "doodle.point");
    double radians = luaL_checknumber(L, 2);
    double offset = luaL_checknumber(L, 3);

    push_point_userdata(L, doodle_point_polar_offset(*p, radians, offset));

    return 1;
}
