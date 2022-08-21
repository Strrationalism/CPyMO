#include "cpymo_prelude.h"
#include <cpymo_backend_text.h>
#include <SDL/SDL.h>
#include <stb_truetype.h>
#include <cpymo_backend_image.h>
#include <cpymo_utils.h>
#include <cpymo_engine.h>

const extern stbtt_fontinfo font;
const extern cpymo_engine engine;
extern SDL_Surface *framebuffer;


#ifndef FONT_RENDER_QUALITY     // 0 ~ 3
#define FONT_RENDER_QUALITY 3
#endif

extern void cpymo_backend_font_render(
    void *out_or_null, int *w, int *h, 
    cpymo_str text, float scale, float baseline);

typedef struct {
    float baseline;
    size_t w, h;
    uint8_t px[0];
} cpymo_backend_text_impl;

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_str s, 
    float height)
{
    float scale = stbtt_ScaleForPixelHeight(&font, height);

    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, NULL, NULL);
    float baseline = scale * ascent;

    int w, h;
    cpymo_backend_font_render(NULL, &w, &h, s, scale, baseline);

    cpymo_backend_text_impl *impl = malloc(sizeof(cpymo_backend_text_impl) + w * h);
    if (impl == NULL) return CPYMO_ERR_OUT_OF_MEM;

    impl->baseline = baseline;
    impl->w = (size_t)w;
    impl->h = (size_t)h;

    memset(impl->px, 0, w * h);
    
    cpymo_backend_font_render(impl->px, &w, &h, s, scale, baseline);
    *out = impl;
    *out_width = (float)w;
    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text t_)
{
    free(t_);
}

static void cpymo_backend_text_draw_internal(
    cpymo_backend_text_impl *t, 
    float x_pos, float y_pos, 
    cpymo_color col, float alpha)
{
    extern cpymo_color getpixel(SDL_Surface *surface, int x, int y);
    extern void putpixel(SDL_Surface *surface, int x, int y, cpymo_color col);

    SDL_Rect clip;
    SDL_GetClipRect(framebuffer, &clip);

    float src_r = col.r / 255.0f;
    float src_g = col.g / 255.0f;
    float src_b = col.b / 255.0f;

    int base_x = (int)x_pos + clip.x;
    int base_y = (int)y_pos + clip.y;

    for (int y = 0; y < t->h; ++y) {
        for (int x = 0; x < t->w; ++x) {
            int draw_x = x + base_x;
            int draw_y = y + base_y;

            if (draw_x < 0 || draw_x >= clip.x + clip.w || draw_y < 0 || draw_y >= clip.y + clip.h) continue;

            const uint8_t fontsmp = t->px[y * t->w + x];

#if (FONT_RENDER_QUALITY == 0 || FONT_RENDER_QUALITY == 1)
            if (!fontsmp) continue;
            putpixel(framebuffer, draw_x, draw_y, col);
#else
            float fontpx = alpha * (fontsmp / 255.0f);
            if (fontpx == 0.0f) continue;
            cpymo_color dst = getpixel(framebuffer, draw_x, draw_y);
            float dst_r = dst.r / 255.0f;
            float dst_g = dst.g / 255.0f;
            float dst_b = dst.b / 255.0f;

            float blend_r = src_r * fontpx + dst_r * (1.0f - fontpx);
            float blend_g = src_g * fontpx + dst_g * (1.0f - fontpx);
            float blend_b = src_b * fontpx + dst_b * (1.0f - fontpx);

            dst.r = (uint8_t)cpymo_utils_clampf(blend_r * 255.0f, 0.0f, 255.0f);
            dst.g = (uint8_t)cpymo_utils_clampf(blend_g * 255.0f, 0.0f, 255.0f);
            dst.b = (uint8_t)cpymo_utils_clampf(blend_b * 255.0f, 0.0f, 255.0f);

            putpixel(framebuffer, draw_x, draw_y, dst);
#endif
        }
    }
}

void cpymo_backend_text_draw(
    cpymo_backend_text t_,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type)
{
    cpymo_backend_text_impl *t = (cpymo_backend_text_impl *)t_;
    float y = y_baseline - t->baseline;

    if (SDL_LockSurface(framebuffer) == -1) return;

#if (FONT_RENDER_QUALITY == 1 || FONT_RENDER_QUALITY == 3)
    cpymo_backend_text_draw_internal(t, x + 1, y + 1, cpymo_color_inv(col), alpha);
#endif
    cpymo_backend_text_draw_internal(t, x, y, col, alpha);

    SDL_UnlockSurface(framebuffer);
}

float cpymo_backend_text_width(
    cpymo_str s,
    float height)
{ 
    float scale = stbtt_ScaleForPixelHeight(&font, height);

    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, NULL, NULL);
    float baseline = scale * ascent;

    int w, h;
    cpymo_backend_font_render(NULL, &w, &h, s, scale, baseline);

    return (float)w;
}


