#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"

#include <lauxlib.h>
#include <lualib.h>

static int cpymo_lua_api_vars_index(lua_State *l)
{
    cpymo_val v = cpymo_vars_get(
        &cpymo_lua_state_get_engine(l)->vars, 
        cpymo_str_pure(lua_tostring(l, -1)));
    lua_pushinteger(l, (lua_Integer)v);
    return 1;
}

static int cpymo_lua_api_vars_newindex(lua_State *l)
{
    int succeed;
    cpymo_val v = (cpymo_val)lua_tointegerx(l, -1, &succeed);
    if (!succeed) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
    const char *k = lua_tostring(l, -2);
    error_t err = cpymo_vars_set(
        &cpymo_lua_state_get_engine(l)->vars, 
        cpymo_str_pure(k), v);
    CPYMO_LUA_THROW(l, err);
    return 0;
}

static void cpymo_lua_context_vars_register(lua_State *l)
{
    lua_newtable(l);
    lua_newtable(l);

    lua_pushcfunction(l, &cpymo_lua_api_vars_index);
    lua_setfield(l, -2, "__index");
    lua_pushcfunction(l, &cpymo_lua_api_vars_newindex);
    lua_setfield(l, -2, "__newindex");

    lua_setmetatable(l, -2);
    lua_setfield(l, -2, "vars");
}

void cpymo_lua_api_script_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;

    lua_newtable(l);

    // basic
    const luaL_Reg funcs[] = {
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);

    cpymo_lua_context_vars_register(ctx->lua_state);

    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "script");
}

#endif