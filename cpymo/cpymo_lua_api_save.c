#include "cpymo_prelude.h"

#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"
#include "cpymo_save_global.h"

#include <lauxlib.h>
#include <lualib.h>
#include "../stb/stb_ds.h"

static int cpymo_lua_api_save_save_global(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 0);

    cpymo_engine *e = cpymo_lua_state_get_engine(l);

    error_t err = cpymo_save_global_save(e);
    CPYMO_LUA_THROW(l, err);

    return 0;
}

error_t cpymo_lua_serialize(cpymo_lua_context *ctx, char **arrbuf);
int cpymo_lua_api_cpymo_deserialize(lua_State *l);

// out_save_str needs free by arrfree(stb_ds.h)
error_t cpymo_lua_context_call_save(cpymo_engine *e, char **out_save_str)
{
    cpymo_lua_get_main_actor(&e->lua);
    if (lua_getfield(e->lua.lua_state, -1, "save") != LUA_TFUNCTION) {
        *out_save_str = NULL;
        arrpush(*out_save_str, 'n');
        arrpush(*out_save_str, 'i');
        arrpush(*out_save_str, 'l');
        arrpush(*out_save_str, '\0');
        lua_pop(e->lua.lua_state, 2);
        return CPYMO_ERR_SUCC;
    }

    error_t err = cpymo_lua_context_execute(&e->lua, 0, 1);
    CPYMO_THROW(err);

    *out_save_str = NULL;
    err = cpymo_lua_serialize(&e->lua, out_save_str);
    arrpush(*out_save_str, '\0');
    if (err != CPYMO_ERR_SUCC) {
        arrfree(*out_save_str);
        return err;
    }

    lua_pop(e->lua.lua_state, 1);
    return CPYMO_ERR_SUCC;
}

error_t cpymo_lua_context_call_load(cpymo_engine *e, const char *str)
{
    cpymo_lua_get_main_actor(&e->lua);
    if (lua_getfield(e->lua.lua_state, -1, "load") != LUA_TFUNCTION) {
        lua_pop(e->lua.lua_state, 1);
        return CPYMO_ERR_SUCC;
    }

    lua_pushcfunction(e->lua.lua_state, &cpymo_lua_api_cpymo_deserialize);
    lua_pushstring(e->lua.lua_state, str);
    error_t err = cpymo_lua_context_execute(&e->lua, 1, 1);
    if (err != CPYMO_ERR_SUCC) {
        lua_pop(e->lua.lua_state, 2);
        return err;
    }

    err = cpymo_lua_context_execute(&e->lua, 1, 0);
    lua_pop(e->lua.lua_state, 1);
    return err;
}

void cpymo_lua_api_save_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;

    // basic
    lua_newtable(l);
    const luaL_Reg funcs[] = {
        { "save_global", &cpymo_lua_api_save_save_global },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);

    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "save");
}

#endif
