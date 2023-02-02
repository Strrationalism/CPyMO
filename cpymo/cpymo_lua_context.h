#ifndef INCLUDE_CPYMO_LUA_CONTEXT
#define INCLUDE_CPYMO_LUA_CONTEXT

#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1

#include <lua.h>
#include "cpymo_error.h"
#include "cpymo_str.h"

struct cpymo_engine;

typedef struct {
    lua_State *lua_state;
    int readonly_metatable;
    struct cpymo_engine *engine;
} cpymo_lua_context;

error_t cpymo_lua_context_init(
    cpymo_lua_context *l, 
    struct cpymo_engine *e);

void cpymo_lua_context_free(cpymo_lua_context *l);

void cpymo_lua_gc_full(cpymo_lua_context *l);

struct cpymo_engine *cpymo_lua_state_get_engine(lua_State *l);

cpymo_lua_context *cpymo_lua_state_get_lua_context(lua_State *l);

error_t cpymo_lua_context_push_lua_code(
    cpymo_lua_context *l, cpymo_str lua_code);

error_t cpymo_lua_context_execute(
    cpymo_lua_context *l, int nargs, int nresults);

error_t cpymo_lua_error_conv(int lua_error);

error_t cpymo_lua_tree_boardcast(
    cpymo_lua_context *l,
    const char *children_table_name,
    const char *preorder_function_name,
    int (*preorder_arg_setter)(lua_State *l, void *userdata),
    void *preorder_arg_setter_userdata,
    const char *postorder_function_name,
    int (*postorder_arg_setter)(lua_State *l, void *userdata),
    void *postorder_arg_setter_userdata);

void cpymo_lua_mark_table_readonly(cpymo_lua_context *l);

error_t cpymo_lua_actor_update(
    cpymo_lua_context *l,
    float delta_time);

void cpymo_lua_actor_draw(
    const cpymo_lua_context *l);

void cpymo_lua_actor_free(
    cpymo_lua_context *l);

void cpymo_lua_get_main_actor(cpymo_lua_context *l);

error_t cpymo_lua_actor_update_main(
    cpymo_lua_context *l,
    float delta_time);
    
void cpymo_lua_actor_draw_main(
    const cpymo_lua_context *l);

#define CPYMO_LUA_PANIC(L, MSG) \
    { \
        luaL_error(L, MSG); \
        return 0; \
    }

#define CPYMO_LUA_ARG_COUNT(L, COUNT) \
    { \
        if (lua_gettop(L) != COUNT) \
            CPYMO_LUA_PANIC(L, "This function needs " #COUNT " argument(s)."); \
    }

#define CPYMO_LUA_THROW(L, X) \
    { if ((X) != CPYMO_ERR_SUCC) CPYMO_LUA_PANIC(L, cpymo_error_message(X)); }

error_t cpymo_lua_pop_float(
    lua_State *l,
    float *v);

error_t cpymo_lua_pop_lightuserdata(
    lua_State *l,
    const void **v);

error_t cpymo_lua_pop_rect(
    lua_State *l,
    float *x,
    float *y,
    float *w,
    float *h);

error_t cpymo_lua_pop_color(
    lua_State *l,
    cpymo_color *color,
    float *alpha);

#ifdef LEAKCHECK
void cpymo_lua_context_leakcheck(cpymo_lua_context *l);
#endif

#endif
#endif
