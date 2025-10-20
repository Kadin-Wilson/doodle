#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <stdbool.h>
#include <stdint.h>

#include "doodle/point.h"
#include "lua_helpers.h"
#include "lua_point.h"

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

    doodle_point *p = lua_newuserdata(L, sizeof *p);
    p->x = x;
    p->y = y;
    luaL_newmetatable(L, "doodle.point");
    lua_setmetatable(L, -2);

    return 1;
}


