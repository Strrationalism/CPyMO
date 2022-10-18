#ifndef INCLUDE_CPYMO_LUA_CONTEXT
#define INCLUDE_CPYMO_LUA_CONTEXT

#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1

#include <lua.h>
#include "cpymo_error.h"
#include "cpymo_str.h"

typedef struct {
    lua_State *lua_state;
} cpymo_lua_context;

error_t cpymo_lua_context_init(cpymo_lua_context *l, unsigned feature_level);
void cpymo_lua_context_free(cpymo_lua_context *l);

error_t cpymo_lua_context_push_lua_code(
    cpymo_lua_context *l, cpymo_str lua_code);

error_t cpymo_lua_context_execute(
    cpymo_lua_context *l, int nargs, int nresults);

error_t cpymo_lua_error_conv(int lua_error);

#endif
#endif
