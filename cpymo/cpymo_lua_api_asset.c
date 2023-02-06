#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"

#include <lauxlib.h>
#include <lualib.h>

error_t cpymo_lua_api_render_class_image_constructor(
    lua_State *l, cpymo_backend_image image, int w, int h);

#define MAKE_LOAD_IMG_FUNC(ASSET_TYPE, FUNC) \
    static int cpymo_lua_api_asset_load_ ## ASSET_TYPE (lua_State *l) \
    { \
        CPYMO_LUA_ARG_COUNT(l, 1); \
        const char *ass_name_cstr = lua_tostring(l, -1); \
        if (ass_name_cstr == NULL) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG); \
        cpymo_str ass_name = cpymo_str_pure(ass_name_cstr); \
        \
        cpymo_engine *e = cpymo_lua_state_get_engine(l); \
        \
        cpymo_backend_image img; \
        int w, h; \
        \
        error_t err = FUNC(&img, &w, &h, ass_name, &e->assetloader); \
        CPYMO_LUA_THROW(l, err); \
        \
        err = cpymo_lua_api_render_class_image_constructor(l, img, w, h); \
        if (err != CPYMO_ERR_SUCC) { \
            cpymo_backend_image_free(img); \
            CPYMO_LUA_THROW(l, err); \
        } \
        \
        return 1; \
    }

MAKE_LOAD_IMG_FUNC(bg, cpymo_assetloader_load_bg_image);
MAKE_LOAD_IMG_FUNC(chara, cpymo_assetloader_load_chara_image);

static error_t cpymo_assetloader_load_system_image_no_mask(
	cpymo_backend_image *i, int *w, int *h, 
    cpymo_str n, const cpymo_assetloader *l)
{ return cpymo_assetloader_load_system_image(i, w, h, n, l, false); }

MAKE_LOAD_IMG_FUNC(system, cpymo_assetloader_load_system_image_no_mask);

#undef MAKE_LOAD_IMG_FUNC

static int cpymo_lua_api_asset_load_system_masktrans(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    const char *masktrans_name = lua_tostring(l, -1);
    if (masktrans_name == NULL) CPYMO_LUA_THROW(l, CPYMO_ERR_INVALID_ARG);

    cpymo_backend_masktrans masktrans;
    error_t err = cpymo_assetloader_load_system_masktrans(
        &masktrans, cpymo_str_pure(masktrans_name), 
        &cpymo_lua_state_get_engine(l)->assetloader);
    CPYMO_LUA_THROW(l, err);

    error_t cpymo_lua_api_render_class_masktrans_constructor(
        lua_State *l, cpymo_backend_masktrans masktrans);
    err = cpymo_lua_api_render_class_masktrans_constructor(l, masktrans);
    if (err != CPYMO_ERR_SUCC) {
        cpymo_backend_masktrans_free(masktrans);
        CPYMO_LUA_THROW(l, err);
    }

    return 1;
}

void cpymo_lua_api_asset_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;
    lua_newtable(l);

    // basic
    const luaL_Reg funcs[] = {
        { "load_bg", &cpymo_lua_api_asset_load_bg },
        { "load_chara", &cpymo_lua_api_asset_load_chara },
        { "load_system_image", &cpymo_lua_api_asset_load_system },
        { "load_system_masktrans", &cpymo_lua_api_asset_load_system_masktrans },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);

    cpymo_lua_mark_table_readonly(ctx);
    lua_setfield(l, -2, "asset");
}

#endif
