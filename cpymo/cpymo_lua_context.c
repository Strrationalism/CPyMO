#include "cpymo_prelude.h"

#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_lua_context.h"

#ifdef LINK_WITH_ONELUA
#define l_unlikely(X) (X)
#define l_likely(X) (X)
#define MAKE_LIB
#include <onelua.c>
#endif

#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>

error_t cpymo_lua_context_init(cpymo_lua_context *l, unsigned feature_level)
{
    if (feature_level < 1) 
    {
        l->lua_state = NULL;
        return CPYMO_ERR_SUCC;
    }

    l->lua_state = luaL_newstate();
    if (l->lua_state == NULL) return CPYMO_ERR_OUT_OF_MEM;

    luaL_openlibs(l->lua_state);
    
    return CPYMO_ERR_SUCC;
}

void cpymo_lua_context_free(cpymo_lua_context *l)
{
    if (l->lua_state) lua_close(l->lua_state);
}

error_t cpymo_lua_error_conv(int lua_error)
{
    switch (lua_error)
    {
        case LUA_OK: return CPYMO_ERR_SUCC;
        case LUA_ERRSYNTAX: return CPYMO_ERR_SYNTAX_ERROR;
        case LUA_ERRMEM: return CPYMO_ERR_OUT_OF_MEM;
        default: return CPYMO_ERR_UNKNOWN;
    }
}

error_t cpymo_lua_context_push_lua_code(
    cpymo_lua_context *l, 
    cpymo_str code)
{
    assert(l->lua_state);
    error_t err = cpymo_lua_error_conv(
        luaL_loadbuffer(l->lua_state, code.begin, code.len, "="));

    if (err != CPYMO_ERR_SUCC)
    {
        const char *errstr = lua_tostring(l->lua_state, -1);
        puts(errstr);
        lua_pop(l->lua_state, 1);
        return err;
    }
    
    return CPYMO_ERR_SUCC;
}

error_t cpymo_lua_context_execute(
    cpymo_lua_context *l, int nargs, int nresults)
{
    assert(l->lua_state);
    error_t err = cpymo_lua_error_conv(
        lua_pcall(l->lua_state, nargs, nresults, 0));

    if (err != CPYMO_ERR_SUCC)
    {
        const char *errstr = lua_tostring(l->lua_state, -1);
        puts(errstr);
        return err;
    }
    
    return CPYMO_ERR_SUCC;
}

#endif
