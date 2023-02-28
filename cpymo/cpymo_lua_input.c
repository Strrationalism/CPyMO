#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"
#include "cpymo_lua_context.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define D_KEYS \
    D_BOOL(mouse_button); \
    D_BOOL(up); D_BOOL(down); D_BOOL(left); D_BOOL(right); \
    D_BOOL(ok); D_BOOL(cancel); D_BOOL(hide_window); D_BOOL(skip); \
    D_BOOL(mouse_position_useable); \
    \
    D_FLOAT(mouse_x); D_FLOAT(mouse_y); D_FLOAT(mouse_wheel_delta); \

// Make key name hash variable
#define MKV(KEY_NAME) \
    static uint64_t cpymo_lua_api_input_keyname_hash_ ## KEY_NAME = 0;
#define D_BOOL(X) MKV(X)
#define D_FLOAT(X) MKV(X)

D_KEYS;

#undef MKV
#undef D_BOOL
#undef D_FLOAT

int cpymo_lua_readonly_table_newindex(lua_State *l);

static int cpymo_lua_api_input_index(lua_State *l)
{
    const char *key = lua_tostring(l, -1);
    if (key == NULL) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
    cpymo_input *input = *(cpymo_input **)lua_touserdata(l, -2);

    uint64_t key_hash = cpymo_str_hash_cstr(key);
    cpymo_lua_context *c = cpymo_lua_state_get_lua_context(l);
    
        
    #define D_BOOL(K) \
        if (key_hash == cpymo_lua_api_input_keyname_hash_ ## K) { \
            lua_pushboolean(l, input->K); \
            return 1; \
        }

    #define D_FLOAT(K) \
        if (key_hash == cpymo_lua_api_input_keyname_hash_ ## K) { \
            lua_pushnumber(l, input->K); \
            return 1; \
        }

    D_KEYS;
    #undef D_BOOL
    #undef D_FLOAT

    CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
}

static int cpymo_lua_api_just_pressed_index(lua_State *l)
{
    const char *key = lua_tostring(l, -1);
    if (key == NULL) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);

    uint64_t key_hash = cpymo_str_hash_cstr(key);
    
    cpymo_engine *e = cpymo_lua_state_get_engine(l);
    #define D_BOOL(K) \
        if (key_hash == cpymo_lua_api_input_keyname_hash_ ## K) { \
            lua_pushboolean(l, CPYMO_INPUT_JUST_PRESSED(e, K)); \
            return 1; \
        }
    #define D_FLOAT(...)
    D_KEYS;
    #undef D_BOOL
    #undef D_FLOAT

    CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
}

static int cpymo_lua_api_just_released_index(lua_State *l)
{
    const char *key = lua_tostring(l, -1);
    if (key == NULL) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);

    uint64_t key_hash = cpymo_str_hash_cstr(key);
    
    cpymo_engine *e = cpymo_lua_state_get_engine(l);
    #define D_BOOL(K) \
        if (key_hash == cpymo_lua_api_input_keyname_hash_ ## K) { \
            lua_pushboolean(l, CPYMO_INPUT_JUST_RELEASED(e, K)); \
            return 1; \
        }
    #define D_FLOAT(...)
    D_KEYS;
    #undef D_BOOL
    #undef D_FLOAT

    CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
}

static int cpymo_lua_api_mouse_moved(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 0);
    cpymo_engine *e = cpymo_lua_state_get_engine(l);
    lua_pushboolean(l, cpymo_input_mouse_moved(e));
    return 1;
}

error_t cpymo_lua_api_input_register(cpymo_lua_context *ctx)
{
    // Make keyname hash value
    #define MKV(KEY_NAME) \
        cpymo_lua_api_input_keyname_hash_ ## KEY_NAME = \
            cpymo_str_hash_cstr(#KEY_NAME);
    #define D_BOOL(X) MKV(X)
    #define D_FLOAT(X) MKV(X)

    D_KEYS;

    #undef MKV
    #undef D_BOOL
    #undef D_FLOAT

    lua_State *l = ctx->lua_state;

    lua_newtable(l);

    {
        lua_newtable(l);

        lua_pushcfunction(l, &cpymo_lua_readonly_table_newindex);
        lua_setfield(l, -2, "__newindex");

        lua_pushcfunction(l, &cpymo_lua_api_input_index);
        lua_setfield(l, -2, "__index");

        int metatable = luaL_ref(l, LUA_REGISTRYINDEX);

        cpymo_input **userdata = (cpymo_input **)lua_newuserdata(l, sizeof(cpymo_input *));
        if (userdata == NULL) return CPYMO_ERR_OUT_OF_MEM;
        *userdata = &ctx->engine->input;
        lua_rawgeti(l, LUA_REGISTRYINDEX, metatable);
        lua_setmetatable(l, -2);
        lua_setfield(l, -2, "now");

        userdata = (cpymo_input **)lua_newuserdata(l, sizeof(cpymo_input *));
        if (userdata == NULL) return CPYMO_ERR_OUT_OF_MEM;
        *userdata = &ctx->engine->prev_input;
        lua_rawgeti(l, LUA_REGISTRYINDEX, metatable);
        lua_setmetatable(l, -2);
        lua_setfield(l, -2, "prev");

        lua_newtable(l);
        lua_newtable(l);
        lua_pushcfunction(l, &cpymo_lua_readonly_table_newindex);
        lua_setfield(l, -2, "__newindex");
        lua_pushcfunction(l, &cpymo_lua_api_just_pressed_index);
        lua_setfield(l, -2, "__index");
        lua_setmetatable(l, -2);
        lua_setfield(l, -2, "just_pressed");

        lua_newtable(l);
        lua_newtable(l);
        lua_pushcfunction(l, &cpymo_lua_readonly_table_newindex);
        lua_setfield(l, -2, "__newindex");
        lua_pushcfunction(l, &cpymo_lua_api_just_released_index);
        lua_setfield(l, -2, "__index");
        lua_setmetatable(l, -2);
        lua_setfield(l, -2, "just_released");

        lua_pushcfunction(l, &cpymo_lua_api_mouse_moved);
        lua_setfield(l, -2, "mouse_moved");

        luaL_unref(l, LUA_REGISTRYINDEX, metatable);
    }
    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "input");

    return CPYMO_ERR_SUCC;
}

#endif
