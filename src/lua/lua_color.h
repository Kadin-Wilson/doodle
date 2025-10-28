#ifndef DOODLE_LUA_COLOR_H
#define DOODLE_LUA_COLOR_H

#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <stdint.h>

int create_color(lua_State *L);
void set_global_colors(lua_State *L);

#endif
