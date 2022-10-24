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
} cpymo_lua_context;

error_t cpymo_lua_context_init(
    cpymo_lua_context *l, 
    struct cpymo_engine *e);
void cpymo_lua_context_free(cpymo_lua_context *l);

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

error_t cpymo_lua_actor_update(
    cpymo_lua_context *l,
    float delta_time);

void cpymo_lua_get_main_actor(cpymo_lua_context *l);

error_t cpymo_lua_actor_update_main(
    cpymo_lua_context *l,
    float delta_time);

#ifdef LEAKCHECK
void cpymo_lua_context_leakcheck(cpymo_lua_context *l);
#endif

#endif
#endif
