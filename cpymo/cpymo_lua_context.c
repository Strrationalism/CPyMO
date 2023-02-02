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
#include <lgc.h>
#include <assert.h>
#include "cpymo_engine.h"

static const char *cpymo_lua_actor_children_table_name = "children";


static inline cpymo_lua_context *cpymo_lua_state_get_lua_context(lua_State *l)
{ return *(cpymo_lua_context **)lua_getextraspace(l); }

static int cpymo_lua_api_cpymo_readonly(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    if (!lua_istable(l, -1)) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
    cpymo_lua_mark_table_readonly(cpymo_lua_state_get_lua_context(l));
    return 1;
}

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
    lua_pushlightuserdata(l, NULL);
    lua_newtable(l);

    lua_pushcfunction(l, &cpymo_lua_api_vars_index);
    lua_setfield(l, -2, "__index");
    lua_pushcfunction(l, &cpymo_lua_api_vars_newindex);
    lua_setfield(l, -2, "__newindex");

    lua_setmetatable(l, -2);
    lua_setfield(l, -2, "vars");
}

static int cpymo_lua_api_is_skipping(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 0);
    lua_pushboolean(
        l, 
        cpymo_engine_skipping(cpymo_lua_state_get_engine(l)));
    return 1;
}

static void cpymo_lua_context_create_cpymo_package(
    cpymo_lua_context *ctx, cpymo_engine *e)
{
    lua_State *l = ctx->lua_state;
    lua_newtable(l);

    lua_pushstring(l, e->assetloader.gamedir);
    lua_setfield(l, -2, "gamedir");

    lua_pushinteger(l, CPYMO_FEATURE_LEVEL);
    lua_setfield(l, -2, "feature_level");

    lua_pushcfunction(l, &cpymo_lua_api_cpymo_readonly);
    lua_setfield(l, -2, "readonly");

    lua_pushcfunction(l, &cpymo_lua_api_is_skipping);
    lua_setfield(l, -2, "is_skipping");

    void cpymo_lua_api_render_register(cpymo_lua_context *);
    cpymo_lua_api_render_register(ctx);

    void cpymo_lua_api_asset_register(cpymo_lua_context *);
    cpymo_lua_api_asset_register(ctx);

    void cpymo_lua_api_ui_register(cpymo_lua_context *);
    cpymo_lua_api_ui_register(ctx);

    void cpymo_lua_api_flags_register(cpymo_lua_context *ctx);
    cpymo_lua_api_flags_register(ctx);

    cpymo_lua_context_vars_register(ctx->lua_state);

    cpymo_lua_mark_table_readonly(ctx);
    lua_setglobal(l, "cpymo");
}

static int cpymo_lua_readonly_table_newindex(lua_State *l)
{
    CPYMO_LUA_PANIC(l, "This table is read-only.");
}

static int cpymo_lua_readonly_table_index(lua_State *l)
{
    const char *key = lua_tostring(l, -1);
    int ref = *(int *)lua_touserdata(l, -2);
    lua_rawgeti(l, LUA_REGISTRYINDEX, ref);
    lua_getfield(l, -1, key);    
    return 1;
}

static int cpymo_lua_readonly_table_len(lua_State *l)
{
    int ref = *(int *)lua_touserdata(l, -1);
    lua_rawgeti(l, LUA_REGISTRYINDEX, ref);
    lua_len(l, -1);
    return 1;
}

static int cpymo_lua_readonly_table_tostring(lua_State *l)
{
    int ref = *(int *)lua_touserdata(l, -1);
    lua_rawgeti(l, LUA_REGISTRYINDEX, ref);
    size_t len;
    luaL_tolstring(l, -1, &len);
    return 1;
}

static int cpymo_lua_readonly_table_gc(lua_State *l)
{
    int ref = *(int *)lua_touserdata(l, -1);
    luaL_unref(l, LUA_REGISTRYINDEX, ref);
    return 0;
}

static int cpymo_lua_readonly_table_pairs(lua_State *l)
{
    int ref = *(int *)lua_touserdata(l, -1);
    lua_pop(l, 1);

    lua_getglobal(l, "pairs");
    lua_rawgeti(l, LUA_REGISTRYINDEX, ref);

    lua_call(l, 1, 3);
    return 3;
}

static int cpymo_lua_context_create_readonly_metatable(lua_State *l)
{
    lua_newtable(l);

    const luaL_Reg funcs[] = {
        { "__newindex", &cpymo_lua_readonly_table_newindex },
        { "__index", &cpymo_lua_readonly_table_index },
        { "__len", &cpymo_lua_readonly_table_len },
        { "__tostring", &cpymo_lua_readonly_table_tostring },
        { "__gc", &cpymo_lua_readonly_table_gc },
        { "__pairs", &cpymo_lua_readonly_table_pairs },
        { NULL, NULL }
    };

    luaL_setfuncs(l, funcs, 0);

    return luaL_ref(l, LUA_REGISTRYINDEX);
}

error_t cpymo_lua_context_init(cpymo_lua_context *l, cpymo_engine *e)
{
    if (e->feature_level < 1) {
        l->lua_state = NULL;
        return CPYMO_ERR_SUCC;
    }

    l->engine = e;

    l->lua_state = luaL_newstate();
    if (l->lua_state == NULL) return CPYMO_ERR_OUT_OF_MEM;
    
    *(cpymo_lua_context **)lua_getextraspace(l->lua_state) = l;

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

void cpymo_lua_gc_full(cpymo_lua_context *l)
{ luaC_fullgc(l->lua_state, true); }

cpymo_engine *cpymo_lua_state_get_engine(lua_State *l)
{ return cpymo_lua_state_get_lua_context(l)->engine; }

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
    int ref = luaL_ref(l->lua_state, LUA_REGISTRYINDEX);
    int *userdata = (int *)lua_newuserdata(l->lua_state, sizeof(ref));
    *userdata = ref;
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
    lua_pop(l, 2);

    if (!succeed[0] || !succeed[1] || !succeed[2] || !succeed[3])
        return CPYMO_ERR_INVALID_ARG;
    
    *x = (float)rect[0];
    *y = (float)rect[1];
    *w = (float)rect[2];
    *h = (float)rect[3];

    return CPYMO_ERR_SUCC;
}

error_t cpymo_lua_pop_color(
    lua_State *l,
    cpymo_color *col,
    float *alpha)
{
    POPCHECK(l);

    int succeed[4];

    if (!lua_istable(l, -1)) return CPYMO_ERR_INVALID_ARG;

    lua_Number color[4];

    lua_getfield(l, -1, "r");
    color[0] = lua_tonumberx(l, -1, succeed);
    lua_pop(l, 1);

    lua_getfield(l, -1, "g");
    color[1] = lua_tonumberx(l, -1, succeed + 1);
    lua_pop(l, 1);

    lua_getfield(l, -1, "b");
    color[2] = lua_tonumberx(l, -1, succeed + 2);
    lua_pop(l, 1);

    succeed[3] = 1;
    if (lua_getfield(l, -1, "a") == LUA_TNIL) color[3] = 255.0f;
    else color[3] = lua_tonumberx(l, -1, succeed + 3);
    lua_pop(l, 2);

    if (!succeed[0] || !succeed[1] || !succeed[2] || !succeed[3])
        return CPYMO_ERR_INVALID_ARG;

    col->r = (uint8_t)color[0];
    col->g = (uint8_t)color[1];
    col->b = (uint8_t)color[2];
    *alpha = color[3] / 255.0f;

    return CPYMO_ERR_SUCC;
}

#endif
