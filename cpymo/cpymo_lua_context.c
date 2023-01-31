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
#include "cpymo_engine.h"

static const char *cpymo_lua_actor_children_table_name = "children";

static void cpymo_lua_context_create_cpymo_package(
    cpymo_lua_context *ctx, cpymo_engine *e)
{
    lua_State *l = ctx->lua_state;
    lua_newtable(l);
    
    lua_pushlightuserdata(l, e);
    lua_setfield(l, -2, "engine");

    lua_pushstring(l, e->assetloader.gamedir);
    lua_setfield(l, -2, "gamedir");

    lua_pushinteger(l, CPYMO_FEATURE_LEVEL);
    lua_setfield(l, -2, "feature_level");

    lua_pushnumber(l, (lua_Number)0);
    lua_setfield(l, -2, "delta_time");

    void cpymo_lua_api_render_register(cpymo_lua_context *);
    cpymo_lua_api_render_register(ctx);

    void cpymo_lua_api_asset_register(cpymo_lua_context *);
    cpymo_lua_api_asset_register(ctx);

    void cpymo_lua_api_ui_register(cpymo_lua_context *);
    cpymo_lua_api_ui_register(ctx);

    cpymo_lua_mark_table_readonly(ctx);
    lua_setglobal(l, "cpymo");
}

static int cpymo_lua_readonly_table_newindex(lua_State *l)
{
    CPYMO_LUA_PANIC(l, "Don\'t write value to cpymo package.");
}

static int cpymo_lua_context_create_readonly_metatable(lua_State *l)
{
    lua_newtable(l);

    lua_pushcfunction(l, &cpymo_lua_readonly_table_newindex);
    lua_setfield(l, -2, "__newindex");

    return luaL_ref(l, LUA_REGISTRYINDEX);
}

error_t cpymo_lua_context_init(cpymo_lua_context *l, cpymo_engine *e)
{
    if (e->feature_level < 1) {
        l->lua_state = NULL;
        return CPYMO_ERR_SUCC;
    }

    l->lua_state = luaL_newstate();
    if (l->lua_state == NULL) return CPYMO_ERR_OUT_OF_MEM;

    luaL_openlibs(l->lua_state);

    l->readonly_metatable = 
        cpymo_lua_context_create_readonly_metatable(l->lua_state);

    cpymo_lua_context_create_cpymo_package(l, e);
    
    return CPYMO_ERR_SUCC;
}

void cpymo_lua_context_free(cpymo_lua_context *l)
{
    if (l->lua_state) {
        cpymo_lua_get_main_actor(l);
        cpymo_lua_actor_free(l);
        lua_close(l->lua_state);
    } 
}

cpymo_engine *cpymo_lua_state_get_engine(lua_State *l)
{
    lua_getglobal(l, "cpymo");
    lua_getfield(l, -1, "engine");
    void *e = lua_touserdata(l, -1);
    lua_pop(l, 2);
    return (cpymo_engine *)e;
}

error_t cpymo_lua_error_conv(int lua_error)
{
    switch (lua_error) {
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

    if (err != CPYMO_ERR_SUCC) {
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

    if (err != CPYMO_ERR_SUCC) {
        const char *errstr = lua_tostring(l->lua_state, -1);
        puts(errstr);
        lua_pop(l->lua_state, 1);
        return err;
    }
    
    return CPYMO_ERR_SUCC;
}

error_t cpymo_lua_tree_boardcast(
    cpymo_lua_context *lcontext,
    const char *children_table_name,
    const char *preorder_function_name,
    int (*preorder_arg_setter)(lua_State *l, void *userdata),
    void *preorder_arg_setter_userdata,
    const char *postorder_function_name,
    int (*postorder_arg_setter)(lua_State *l, void *userdata),
    void *postorder_arg_setter_userdata)
{
    lua_State *l = lcontext->lua_state;

    if (!lua_istable(l, -1)) return CPYMO_ERR_SUCC;

    lua_getfield(l, -1, preorder_function_name);
    if (lua_isfunction(l, -1)) {
        int nargs = 1;
        lua_pushvalue(l, -2);
        if (preorder_arg_setter)
            nargs = 1 + preorder_arg_setter(l, preorder_arg_setter_userdata);
        error_t err = cpymo_lua_context_execute(lcontext, nargs, 0);
        CPYMO_THROW(err);
    }
    else {
        lua_pop(l, 1);
    }

    if (children_table_name) {
        lua_getfield(l, -1, children_table_name);
        if (lua_istable(l, -1)) {
            lua_pushnil(l);
            while (lua_next(l, -2)) {
                error_t err = cpymo_lua_tree_boardcast(
                    lcontext,
                    children_table_name,
                    preorder_function_name,
                    preorder_arg_setter,
                    preorder_arg_setter_userdata,
                    postorder_function_name,
                    postorder_arg_setter,
                    postorder_arg_setter_userdata);
                lua_pop(l, 1);
            }
        }
        lua_pop(l, 1);
    }
    
    lua_getfield(l, -1, postorder_function_name);
    if (lua_isfunction(l, -1)) {
        int nargs = 1;
        lua_pushvalue(l, -2);
        if (postorder_arg_setter)
            nargs = 1 + postorder_arg_setter(l, postorder_arg_setter_userdata);
        error_t err = cpymo_lua_context_execute(lcontext, nargs, 0);
        CPYMO_THROW(err);
    }
    else {
        lua_pop(l, 1);
    }

    return CPYMO_ERR_SUCC;
}

void cpymo_lua_mark_table_readonly(cpymo_lua_context *l)
{
    lua_rawgeti(l->lua_state, LUA_REGISTRYINDEX, l->readonly_metatable);
    lua_setmetatable(l->lua_state, -2);
}

static int cpymo_lua_actor_update_arg_setter(
    lua_State *s, void *p_float_deltatime)
{
    lua_pushnumber(s, (lua_Number)(*((float *)p_float_deltatime)));
    return 1;
}

error_t cpymo_lua_actor_update(
    cpymo_lua_context *l,
    float delta_time)
{
    return cpymo_lua_tree_boardcast(
        l,
        cpymo_lua_actor_children_table_name,
        "update",
        &cpymo_lua_actor_update_arg_setter,
        (void *)&delta_time,
        "late_update",
        &cpymo_lua_actor_update_arg_setter,
        (void *)&delta_time);
}

void cpymo_lua_actor_draw(
    const cpymo_lua_context *l)
{
    cpymo_lua_tree_boardcast(
        (cpymo_lua_context *)l,
        cpymo_lua_actor_children_table_name,
        "draw", NULL, NULL,
        "late_draw", NULL, NULL);
}

void cpymo_lua_actor_free(
    cpymo_lua_context *l)
{
    cpymo_lua_tree_boardcast(
        l,
        cpymo_lua_actor_children_table_name,
        "free", NULL, NULL,
        "late_free", NULL, NULL);
}

void cpymo_lua_get_main_actor(cpymo_lua_context *l)
{
    lua_getglobal(l->lua_state, "main");
}

error_t cpymo_lua_actor_update_main(
    cpymo_lua_context *l,
    float delta_time)
{
    if (l->lua_state == NULL) return CPYMO_ERR_SUCC;
    cpymo_lua_get_main_actor(l);
    error_t err = cpymo_lua_actor_update(l, delta_time);
    #ifdef LEAKCHECK
    lua_pop(l->lua_state, 1);
    #else
    lua_pop(l->lua_state, -1);
    #endif
    return err;
}

void cpymo_lua_actor_draw_main(
    const cpymo_lua_context *l)
{
    if (l->lua_state == NULL) return;
    cpymo_lua_get_main_actor((cpymo_lua_context *)l);
    cpymo_lua_actor_draw(l);
    #ifdef LEAKCHECK
    lua_pop(l->lua_state, 1);
    #else
    lua_pop(l->lua_state, -1);
    #endif
}

#ifdef LEAKCHECK
void cpymo_lua_context_leakcheck(cpymo_lua_context *l)
{
    if (l->lua_state == NULL) return;
    int num = lua_gettop(l->lua_state);
    if (!num) return;

    printf("[LEAKCHECK] Lua stack leaking detected: %d objects leaked.\n", num);
}
#endif

#define POPCHECK(L) \
    if (lua_gettop(L) < 0) return CPYMO_ERR_INVALID_ARG;

error_t cpymo_lua_pop_float(
    lua_State *l,
    float *v)
{
    POPCHECK(l);

    int succeed;
    lua_Number x = lua_tonumberx(l, -1, &succeed);
    if (!succeed) return CPYMO_ERR_INVALID_ARG;

    lua_pop(l, 1);

    if (succeed) *v = (float)x;

    return CPYMO_ERR_SUCC;
}

error_t cpymo_lua_pop_lightuserdata(
    lua_State *l,
    const void **v)
{
    POPCHECK(l);

    if (lua_type(l, -1) != LUA_TLIGHTUSERDATA)
        return CPYMO_ERR_INVALID_ARG;
    
    *v = lua_topointer(l, -1);
    lua_pop(l, 1);

    return CPYMO_ERR_SUCC;
}

error_t cpymo_lua_pop_rect(
    lua_State *l,
    float *x,
    float *y,
    float *w,
    float *h)
{
    POPCHECK(l);
    
    int succeed[4];

    if (!lua_istable(l, -1)) 
        return CPYMO_ERR_INVALID_ARG;

    
    lua_Number rect[4];

    lua_getfield(l, -1, "x");
    rect[0] = lua_tonumberx(l, -1, succeed);
    lua_pop(l, 1);

    lua_getfield(l, -1, "y");
    rect[1] = lua_tonumberx(l, -1, succeed + 1);
    lua_pop(l, 1);

    lua_getfield(l, -1, "w");
    rect[2] = lua_tonumberx(l, -1, succeed + 2);
    lua_pop(l, 1);

    lua_getfield(l, -1, "h");
    rect[3] = lua_tonumberx(l, -1, succeed + 3);
    lua_pop(l, 1);

    if (!succeed[0] || !succeed[1] || !succeed[2] || !succeed[3])
        return CPYMO_ERR_INVALID_ARG;
    
    *x = (float)rect[0];
    *y = (float)rect[1];
    *w = (float)rect[2];
    *h = (float)rect[3];
    lua_pop(l, 1);

    return CPYMO_ERR_SUCC;
}

#endif
