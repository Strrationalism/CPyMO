#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"

#include <lauxlib.h>
#include <lualib.h>

error_t cpymo_lua_api_render_class_image_constructor(
    lua_State *l, cpymo_backend_image image, int w, int h);

static int cpymo_lua_api_asset_load_bg(lua_State *l)
{
    cpymo_str bg_name = cpymo_str_pure(lua_tostring(l, -1));
    lua_pop(l, 1);

    cpymo_engine *e = cpymo_lua_state_get_engine(l);

    cpymo_backend_image img;
    int w, h;

    error_t err = cpymo_assetloader_load_bg_image(
        &img, &w, &h, bg_name, &e->assetloader);
    CPYMO_LUA_THROW(l, err);

    err = cpymo_lua_api_render_class_image_constructor(l, img, w, h);
    if (err != CPYMO_ERR_SUCC) {
        cpymo_backend_image_free(img);
        CPYMO_LUA_THROW(l, err);
    }
    
    return 1;
}

void cpymo_lua_api_asset_register(lua_State *l)
{
    lua_newtable(l);

    // basic
    const luaL_Reg funcs[] = {
        { "load_bg", &cpymo_lua_api_asset_load_bg },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);

    lua_setfield(l, -2, "asset");
}

#endif