#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"

#include <string.h>
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

static int cpymo_lua_api_script_push_code(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    
    const char *pymo_code = lua_tostring(l, -1);
    size_t pymo_code_len = strlen(pymo_code);

    char *target_code = (char *)malloc(pymo_code_len + 6);
    if (target_code == NULL) CPYMO_LUA_THROW(l, CPYMO_ERR_OUT_OF_MEM);
    memcpy(target_code, pymo_code, pymo_code_len);
    memcpy(target_code + pymo_code_len, "\n#ret", 6);

    cpymo_script *script = (cpymo_script *)malloc(sizeof(cpymo_script) + 1);
    if (script == NULL) {
        free(target_code);
        CPYMO_LUA_THROW(l, CPYMO_ERR_OUT_OF_MEM);
    }

    script->script_content = target_code;
    script->script_content_len = pymo_code_len + 5;
    script->script_name[0] = '\0';

    cpymo_interpreter *callee = 
			(cpymo_interpreter *)malloc(sizeof(cpymo_interpreter));

    if (callee == NULL) {
        cpymo_script_free(script);
        CPYMO_LUA_THROW(l, CPYMO_ERR_OUT_OF_MEM);
    }

    cpymo_engine *e = cpymo_lua_state_get_engine(l);
    cpymo_interpreter_init(callee, script, true, e->interpreter);
    e->interpreter = callee;

    return 0;
}


static bool cpymo_lua_api_script_wait_wait(cpymo_engine *e, float d)
{
    lua_rawgeti(
        e->lua.lua_state, 
        LUA_REGISTRYINDEX, 
        e->lua.script_wait_function_id);

    lua_pushnumber(e->lua.lua_state, d);
    error_t err = cpymo_lua_context_execute(&e->lua, 1, 1);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] %s.", cpymo_error_message(err));
        return false;
    }

    bool ret = (bool)lua_toboolean(e->lua.lua_state, -1);
    lua_pop(e->lua.lua_state, 1);
    return ret;
}

static error_t cpymo_lua_api_script_wait_callback(cpymo_engine *e)
{
    lua_State *l = e->lua.lua_state;
    lua_rawgeti(l, LUA_REGISTRYINDEX, e->lua.script_wait_callback_id);
    luaL_unref(l, LUA_REGISTRYINDEX, e->lua.script_wait_function_id);
    luaL_unref(l, LUA_REGISTRYINDEX, e->lua.script_wait_callback_id);
    e->lua.script_wait_function_id = LUA_REFNIL;
    e->lua.script_wait_callback_id = LUA_REFNIL;

    if (lua_isnil(l, -1)) {
        lua_pop(l, 1);
        return CPYMO_ERR_SUCC;
    }
    else 
        return cpymo_lua_context_execute(&e->lua, 0, 0);
}

static int cpymo_lua_api_script_wait(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 2);
    if (!(lua_isfunction(l, -1) || lua_isnil(l, -1)))
        CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
    if (!lua_isfunction(l, -2)) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);

    cpymo_lua_context *ctx = cpymo_lua_state_get_lua_context(l);

    ctx->script_wait_callback_id = luaL_ref(l, LUA_REGISTRYINDEX);
    ctx->script_wait_function_id = luaL_ref(l, LUA_REGISTRYINDEX);

    cpymo_wait_register_with_callback(
        &ctx->engine->wait,
        &cpymo_lua_api_script_wait_wait,
        &cpymo_lua_api_script_wait_callback);

    return 0;
}

error_t cpymo_lua_context_pymo_override(
    cpymo_lua_context *ctx,
    cpymo_str command,
    cpymo_parser *pymo_parser)
{
    if (ctx->lua_state == NULL) return CPYMO_ERR_NOT_FOUND;

    lua_rawgeti(
        ctx->lua_state, 
        LUA_REGISTRYINDEX, 
        ctx->script_commands_table_ref);

    if (lua_pushlstring(ctx->lua_state, command.begin, command.len) == NULL)
        return CPYMO_ERR_OUT_OF_MEM;

    int type = lua_gettable(ctx->lua_state, -2);
    if (type == LUA_TNIL) {
        lua_pop(ctx->lua_state, 2);
        return CPYMO_ERR_NOT_FOUND;
    }
    else if (type != LUA_TFUNCTION) {
        lua_pop(ctx->lua_state, 2);
        puts("[Error] Command handler is not a function.");
        return CPYMO_ERR_INVALID_ARG;
    }

    int args = 0;
    while (cpymo_parser_curline_peek(pymo_parser) != '\0')
    {
        cpymo_str arg = 
            cpymo_parser_curline_pop_commacell(pymo_parser);
        if (lua_pushlstring(ctx->lua_state, arg.begin, arg.len) == NULL) {
            lua_pop(ctx->lua_state, args + 2);
            return CPYMO_ERR_OUT_OF_MEM;
        }

        args++;
    }
    
    error_t err = cpymo_lua_context_execute(ctx, args, 0);
    lua_pop(ctx->lua_state, 1);
    return err;
}

void cpymo_lua_api_script_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;

    lua_newtable(l);

    // basic
    const luaL_Reg funcs[] = {
        { "push_code", &cpymo_lua_api_script_push_code },
        { "wait", &cpymo_lua_api_script_wait },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);

    cpymo_lua_context_vars_register(ctx->lua_state);

    lua_newtable(ctx->lua_state);
    ctx->script_commands_table_ref = 
        luaL_ref(ctx->lua_state, LUA_REGISTRYINDEX);
    lua_rawgeti(
        ctx->lua_state, 
        LUA_REGISTRYINDEX, 
        ctx->script_commands_table_ref);
    lua_setfield(ctx->lua_state, -2, "commands");

    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "script");
}

#endif