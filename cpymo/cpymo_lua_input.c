#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"
#include "cpymo_lua_context.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Make key name hash variable
#define MKV(KEY_NAME) \
    static uint64_t cpymo_lua_api_input_keyname_hash_ ## KEY_NAME = 0;

MKV(up); MKV(down); MKV(left); MKV(right);
MKV(mouse_position_useable); MKV(mouse_x); MKV(mouse_y);
MKV(mouse_button); MKV(mouse_wheel_delta);
MKV(ok); MKV(cancel); MKV(hide_window); MKV(skip);

#undef MKV

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

    D_BOOL(mouse_button);
    D_BOOL(up); D_BOOL(down); D_BOOL(left); D_BOOL(right);
    D_BOOL(ok); D_BOOL(cancel); D_BOOL(hide_window); D_BOOL(skip);
    D_BOOL(mouse_position_useable);

    D_FLOAT(mouse_x); D_FLOAT(mouse_y); D_FLOAT(mouse_wheel_delta);
    #undef D_BOOL
    #undef D_FLOAT

    CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);
}

error_t cpymo_lua_api_input_register(cpymo_lua_context *ctx)
{
    // Make keyname hash value
    #define MKV(KEY_NAME) \
        cpymo_lua_api_input_keyname_hash_ ## KEY_NAME = \
            cpymo_str_hash_cstr(#KEY_NAME);

    MKV(up); MKV(down); MKV(left); MKV(right);
    MKV(mouse_position_useable); MKV(mouse_x); MKV(mouse_y);
    MKV(mouse_button); MKV(mouse_wheel_delta);
    MKV(ok); MKV(cancel); MKV(hide_window); MKV(skip);

    #undef MKV

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

        luaL_unref(l, LUA_REGISTRYINDEX, metatable);
    }
    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "input");

    return CPYMO_ERR_SUCC;
}

#endif
