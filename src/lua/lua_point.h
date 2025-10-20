#ifndef DOODLE_LUA_POINT_H
#define DOODLE_LUA_POINT_H

#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

int create_point(lua_State *L);

int add_points(lua_State *L);
int subtract_points(lua_State *L);
int scale_point(lua_State *L);
int polar_offset_point(lua_State *L);

#endif

