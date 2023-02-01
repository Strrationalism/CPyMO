#include "cpymo_prelude.h"
#if CPYMO_FEATURE_LEVEL >= 1
#include "cpymo_engine.h"

#include <lauxlib.h>
#include <lualib.h>

static int cpymo_lua_api_render_request_redraw(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 0);
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
    if (img == NULL) return CPYMO_ERR_OUT_OF_MEM;

    img->image = image;
    img->w = w;
    img->h = h;
    luaL_getmetatable(l, "cpymo_render_image");
    lua_setmetatable(l, -2);

    return 0;
}

static int cpymo_lua_api_render_class_image_get_size(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
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

static int cpymo_lua_api_render_class_image_draw(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 5);
    const void *draw_type;
    error_t err = cpymo_lua_pop_lightuserdata(l, &draw_type);
    CPYMO_LUA_THROW(l, err);

    float alpha;
    err = cpymo_lua_pop_float(l, &alpha);
    CPYMO_LUA_THROW(l, err);

    float srcx = 0, srcy = 0, srcw, srch;
    bool src_is_nil = lua_isnil(l, -1);
    if (!src_is_nil) {
        err = cpymo_lua_pop_rect(l, &srcx, &srcy, &srcw, &srch);
        CPYMO_LUA_THROW(l, err);
    }
    else lua_pop(l, 1);

    float dstx = 0, dsty = 0, dstw, dsth;
    err = cpymo_lua_pop_rect(l, &dstx, &dsty, &dstw, &dsth);
    CPYMO_LUA_THROW(l, err);
    
    cpymo_lua_api_render_class_image *img =
        (cpymo_lua_api_render_class_image *)lua_touserdata(l, -1);
    lua_pop(l, 1);

    if (src_is_nil) {
        srcw = (float)img->w;
        srch = (float)img->h;
    }

    cpymo_backend_image_draw(
        dstx, dsty, dstw, dsth, img->image,
        srcx, srcy, srcw, srch, alpha / 255.0f, 
        (enum cpymo_backend_image_draw_type)draw_type);

    return 0;
}

static int cpymo_lua_api_render_class_image_free(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 1);
    cpymo_lua_api_render_class_image *img = 
        (cpymo_lua_api_render_class_image *)lua_touserdata(l, 1);
    
    if (img) {
        img->w = 0;
        img->h = 0;
        if (img->image) 
            cpymo_backend_image_free(img->image);
        img->image = NULL;
    }

    return 0;
}

static int cpymo_lua_api_render_fill_rect(lua_State *l)
{
    CPYMO_LUA_ARG_COUNT(l, 3);

    const void *draw_type;
    error_t err = cpymo_lua_pop_lightuserdata(l, &draw_type);
    CPYMO_LUA_THROW(l, err);

    cpymo_color col;
    float alpha;
    err = cpymo_lua_pop_color(l, &col, &alpha);
    CPYMO_LUA_THROW(l, err);

    float r[4];
    err = cpymo_lua_pop_rect(l, r, r + 1, r + 2, r + 3);
    CPYMO_LUA_THROW(l, err);

    cpymo_backend_image_fill_rects(r, 1, col, alpha, 
        (enum cpymo_backend_image_draw_type)draw_type);
    return 0;
}

void cpymo_lua_api_render_register(cpymo_lua_context *ctx)
{
    lua_State *l = ctx->lua_state;

    // class `cpymo_render_image`
    luaL_newmetatable(l, "cpymo_render_image");
    lua_newtable(l);
    const luaL_Reg funcs_class_image[] = {
        { "get_size", &cpymo_lua_api_render_class_image_get_size },
        { "draw", &cpymo_lua_api_render_class_image_draw },
        { "free", &cpymo_lua_api_render_class_image_free },
        { NULL, NULL }
    };
    luaL_setfuncs(l, funcs_class_image, 0);
    lua_setfield(l, -2, "__index");
    lua_pushcfunction(l, &cpymo_lua_api_render_class_image_free);
    lua_setfield(l, -2, "__gc");
    lua_pushcfunction(l, &cpymo_lua_api_render_class_image_free);
    lua_setfield(l, -2, "__close");
    lua_pop(l, 1);

    // package `cpymo.render`
    lua_newtable(l); {
        const luaL_Reg funcs[] = {
            { "request_redraw", &cpymo_lua_api_render_request_redraw },
            { "fill_rect", &cpymo_lua_api_render_fill_rect },
            { NULL, NULL }
        };
        luaL_setfuncs(l, funcs, 0);

        lua_newtable(l); {
            #define BIND(DRAW_TYPE) \
                lua_pushlightuserdata( \
                    l, \
                    (void *)cpymo_backend_image_draw_type_ ## DRAW_TYPE); \
                lua_setfield(l, -2, #DRAW_TYPE);

            BIND(bg);
            BIND(chara);
            BIND(ui_bg);
            BIND(ui_element_bg);
            BIND(ui_element);
            BIND(text_say);
            BIND(text_say_textbox);
            BIND(text_text);
            BIND(titledate_bg);
            BIND(titledate_text);
            BIND(sel_img);
            BIND(effect);

            #undef BIND

            cpymo_lua_mark_table_readonly(ctx);
        } lua_setfield(l, -2, "semantic");
    
        cpymo_lua_mark_table_readonly(ctx);
    } lua_setfield(l, -2, "render");
}


#endif
