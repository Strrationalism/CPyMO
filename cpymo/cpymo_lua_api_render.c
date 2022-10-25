#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"

#include <lauxlib.h>
#include <lualib.h>

static int cpymo_lua_api_render_request_redraw(lua_State *l)
{
    cpymo_engine *e = cpymo_lua_state_get_engine(l);
    if (e) cpymo_engine_request_redraw(e);
    return 0;
}

typedef struct {
    cpymo_backend_image image;
    int w, h;
} cpymo_lua_api_render_class_image;

error_t cpymo_lua_api_render_class_image_constructor(
    lua_State *l, cpymo_backend_image image, int w, int h)
{
    cpymo_lua_api_render_class_image *img = lua_newuserdata(
        l, sizeof(cpymo_lua_api_render_class_image));
    img->image = image;
    img->w = w;
    img->h = h;
    luaL_getmetatable(l, "cpymo_render_image");
    lua_setmetatable(l, -2);

    return 0;
}

static int cpymo_lua_api_render_class_image_get_size(lua_State *l)
{
    cpymo_lua_api_render_class_image *img = 
        (cpymo_lua_api_render_class_image *)lua_touserdata(l, -1);

    int w = 0;
    int h = 0;

    if (img) {
        w = img->w;
        h = img->h;
    }

    lua_pop(l, 1);
    lua_pushinteger(l, w);
    lua_pushinteger(l, h);
    return 2;
}

static int cpymo_lua_api_render_class_image_free(lua_State *l)
{
    cpymo_lua_api_render_class_image *img = 
        (cpymo_lua_api_render_class_image *)lua_touserdata(l, -1);
    
    if (img) {
        img->w = 0;
        img->h = 0;
        if (img->image) 
            cpymo_backend_image_free(img->image);
        img->image = NULL;
    }

    return 0;
}

void cpymo_lua_api_render_register(lua_State *l)
{
    // class `image`
    luaL_newmetatable(l, "cpymo_render_image");
    lua_newtable(l);
    const luaL_Reg funcs_class_image[] = {
        { "get_size", &cpymo_lua_api_render_class_image_get_size },
        { "free", &cpymo_lua_api_render_class_image_free },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs_class_image, 0);
    lua_setfield(l, -2, "__index");
    lua_pushcfunction(l, &cpymo_lua_api_render_class_image_free);
    lua_setfield(l, -2, "__gc");
    lua_pop(l, 1);

    lua_newtable(l);

    // basic
    const luaL_Reg funcs[] = {
        { "request_redraw", &cpymo_lua_api_render_request_redraw },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs, 0);
    
    lua_setfield(l, -2, "render");
}


#endif
