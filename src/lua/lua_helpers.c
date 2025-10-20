#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <stdio.h>
#include <stdbool.h>

#include "lua_helpers.h"

void dumpstack(lua_State *L) {
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

bool geti_number(lua_State *L, int i, double *n) {
    lua_rawgeti(L, 1, i);
    if (lua_isnumber(L, -1)) {
        *n = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

bool getf_number(lua_State *L, const char *key, double *n) {
    lua_getfield(L, 1, key);
    if (lua_isnumber(L, -1)) {
        *n = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

bool has_metatable(lua_State *L, const char *name) {
    if (!lua_getmetatable(L, -1)) {
        return false;
    }
    luaL_getmetatable(L, name);
    bool has = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return has;
}

