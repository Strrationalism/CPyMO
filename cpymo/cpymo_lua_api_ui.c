#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"

#include <lauxlib.h>
#include <lualib.h>

typedef struct {
    int actor_ref;
} cpymo_lua_ui;

static void cpymo_lua_ui_draw(const cpymo_engine *e, const void *ui)
{
    const cpymo_lua_ui *lua_ui = (const cpymo_lua_ui *)ui;
    lua_rawgeti(e->lua.lua_state, LUA_REGISTRYINDEX, lua_ui->actor_ref);
    cpymo_lua_actor_draw(&e->lua);
    lua_pop(e->lua.lua_state, 1);
}

static error_t cpymo_lua_ui_update(cpymo_engine *e, void *ui, float d)
{
    const cpymo_lua_ui *lua_ui = (const cpymo_lua_ui *)ui;
    lua_rawgeti(e->lua.lua_state, LUA_REGISTRYINDEX, lua_ui->actor_ref);
    error_t err = cpymo_lua_actor_update(&e->lua, d);
    lua_pop(e->lua.lua_state, 1);
    return err;
}

static void cpymo_lua_ui_delete(cpymo_engine *e, void *ui)
{
    cpymo_lua_ui *lua_ui = (cpymo_lua_ui *)ui;
    lua_rawgeti(e->lua.lua_state, LUA_REGISTRYINDEX, lua_ui->actor_ref);
    cpymo_lua_actor_free(&e->lua);
    luaL_unref(e->lua.lua_state, LUA_REGISTRYINDEX, lua_ui->actor_ref);
    lua_pop(e->lua.lua_state, 1);
}

static int cpymo_lua_api_ui_enter(lua_State *l)
{
    cpymo_engine *e = cpymo_lua_state_get_engine(l);
    cpymo_lua_ui *ui = NULL;

    error_t err = cpymo_ui_enter(
        (void **)&ui, e, sizeof(cpymo_lua_ui), 
        &cpymo_lua_ui_update,
        &cpymo_lua_ui_draw,
        &cpymo_lua_ui_delete);
    CPYMO_LUA_THROW(l, err);

    if (!lua_istable(l, -1)) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
    ui->actor_ref = luaL_ref(l, LUA_REGISTRYINDEX);

    return 0;
}

static int cpymo_lua_api_ui_exit(lua_State *l)
{ 
    cpymo_ui_exit(cpymo_lua_state_get_engine(l)); 
    return 0;
}

void cpymo_lua_api_ui_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;

    lua_newtable(l);

    // basic
    const luaL_Reg funcs[] = {
        { "enter", &cpymo_lua_api_ui_enter },
        { "exit", &cpymo_lua_api_ui_exit },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);

    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "ui");
}

#endif
