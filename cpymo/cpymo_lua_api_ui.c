#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"
#include "cpymo_msgbox_ui.h"

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
    CPYMO_LUA_ARG_COUNT(l, 1);
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
    CPYMO_LUA_ARG_COUNT(l, 0);
    cpymo_ui_exit(cpymo_lua_state_get_engine(l)); 
    return 0;
}

static int cpymo_lua_api_ui_msgbox(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    if (!lua_isstring(l, -1)) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
    error_t err = cpymo_msgbox_ui_enter(
        cpymo_lua_state_get_engine(l),
        cpymo_str_pure(lua_tostring(l, -1)), 
        NULL, NULL);
    CPYMO_LUA_THROW(l, err);
    return 0;
}

typedef struct {
    lua_State *l;
    int callback_ref;
} cpymo_lua_api_ui_okcancelbox_data;

static error_t cpymo_lua_api_ui_okcancelbox_cb(cpymo_engine *e, void *x, bool c)
{
    cpymo_lua_api_ui_okcancelbox_data d = 
        *(cpymo_lua_api_ui_okcancelbox_data *)x;
    free(x);
    
    lua_rawgeti(d.l, LUA_REGISTRYINDEX, d.callback_ref);
    luaL_unref(d.l, LUA_REGISTRYINDEX, d.callback_ref);
    lua_pushboolean(d.l, c);
    return cpymo_lua_context_execute(
        cpymo_lua_state_get_lua_context(d.l), 1, 0);
}

static int cpymo_lua_api_ui_okcancelbox(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 2);
    if (!lua_isfunction(l, -1)) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
    if (!lua_isstring(l, -2)) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);

    const char *msg = lua_tostring(l, -2);

    cpymo_lua_api_ui_okcancelbox_data *data = 
        malloc(sizeof(cpymo_lua_api_ui_okcancelbox_data));
    if (data == NULL) CPYMO_LUA_THROW(l, CPYMO_ERR_OUT_OF_MEM);

    error_t err = cpymo_msgbox_ui_enter(
        cpymo_lua_state_get_engine(l),
        cpymo_str_pure(msg),
        &cpymo_lua_api_ui_okcancelbox_cb,
        (void *)data);
    if (err != CPYMO_ERR_SUCC) {
        free(data);
        CPYMO_LUA_THROW(l, err);
    }

    data->l = l;
    data->callback_ref = luaL_ref(l, LUA_REGISTRYINDEX);

    return CPYMO_ERR_SUCC;
}

void cpymo_lua_api_ui_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;

    lua_newtable(l);

    // basic
    const luaL_Reg funcs[] = {
        { "enter", &cpymo_lua_api_ui_enter },
        { "exit", &cpymo_lua_api_ui_exit },
        { "msgbox", &cpymo_lua_api_ui_msgbox },
        { "okcancelbox", &cpymo_lua_api_ui_okcancelbox },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);

    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "ui");
}

#endif
