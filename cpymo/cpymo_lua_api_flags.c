#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"
#include "cpymo_album.h"

#include <lauxlib.h>
#include <lualib.h>

static int cpymo_lua_api_flags_set(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    const char *x = lua_tostring(l, -1);
    if (x == NULL) return CPYMO_ERR_INVALID_ARG;

    uint64_t hash;
    cpymo_str_hash_init(&hash);
    cpymo_str_hash_append_cstr(&hash, x);

    error_t err = cpymo_hash_flags_add(
        &cpymo_lua_state_get_engine(l)->flags,
        hash);
    CPYMO_LUA_THROW(l, err);

    return 0;
}

static int cpymo_lua_api_flags_unset(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    const char *x = lua_tostring(l, -1);
    if (x == NULL) return CPYMO_ERR_INVALID_ARG;

    uint64_t hash;
    cpymo_str_hash_init(&hash);
    cpymo_str_hash_append_cstr(&hash, x);

    cpymo_hash_flags_del(
        &cpymo_lua_state_get_engine(l)->flags,
        hash);

    return 0;
}

static int cpymo_lua_api_flags_check(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    const char *x = lua_tostring(l, -1);
    if (x == NULL) return CPYMO_ERR_INVALID_ARG;

    uint64_t hash;
    cpymo_str_hash_init(&hash);
    cpymo_str_hash_append_cstr(&hash, x);

    bool checked = cpymo_hash_flags_check(
        &cpymo_lua_state_get_engine(l)->flags,
        hash);

    lua_pushboolean(l, checked);
    return 1;    
}


static int cpymo_lua_api_flags_unlock_cg(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    const char *x = lua_tostring(l, -1);
    if (x == NULL) return CPYMO_ERR_INVALID_ARG;
    error_t err = cpymo_album_cg_unlock(
        cpymo_lua_state_get_engine(l), cpymo_str_pure(x));
    CPYMO_THROW(err);
    return 0;
}

static int cpymo_lua_api_flags_lock_cg(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    const char *x = lua_tostring(l, -1);
    if (x == NULL) return CPYMO_ERR_INVALID_ARG;

    cpymo_hash_flags_del(
        &cpymo_lua_state_get_engine(l)->flags,
        cpymo_album_cg_name_hash(cpymo_str_pure(x)));

    return 0;
}

static int cpymo_lua_api_flags_cg_unlocked(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    const char *x = lua_tostring(l, -1);
    if (x == NULL) return CPYMO_ERR_INVALID_ARG;

    bool checked = cpymo_hash_flags_check(
        &cpymo_lua_state_get_engine(l)->flags,
        cpymo_album_cg_name_hash(cpymo_str_pure(x)));

    lua_pushboolean(l, checked);
    return 1;    
}

void cpymo_lua_api_flags_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;
    lua_newtable(l);

    const luaL_Reg funcs[] = {
        { "set", &cpymo_lua_api_flags_set },
        { "unset", &cpymo_lua_api_flags_unset },
        { "check", &cpymo_lua_api_flags_check },
        { "unlock_cg", &cpymo_lua_api_flags_unlock_cg },
        { "lock_cg", &cpymo_lua_api_flags_lock_cg },
        { "cg_unlocked", &cpymo_lua_api_flags_cg_unlocked },
        { NULL, NULL }
    };

    luaL_setfuncs(l, funcs, 0);
    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "flags");
}

#endif
