#ifndef DOODLE_LUA_HELPERS_H
#define DOODLE_LUA_HELPERS_H

#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <stdbool.h>

static const char *NOT_PROVIDED = "%s error: failed to provide a value for %s";

void dumpstack(lua_State *L);
bool has_metatable(lua_State *L, const char *name);

bool getf_number(lua_State *L, const char *key, double *n);
bool geti_number(lua_State *L, int i, double *n);

#endif
