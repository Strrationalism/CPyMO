#include "cpymo_prelude.h"

#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"
#include "cpymo_save_global.h"

#include <lauxlib.h>
#include <lualib.h>

static int cpymo_lua_api_save_save_global(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 0);

    cpymo_engine *e = cpymo_lua_state_get_engine(l);

    error_t err = cpymo_save_global_save(e);
    CPYMO_LUA_THROW(l, err);

    return 0;
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