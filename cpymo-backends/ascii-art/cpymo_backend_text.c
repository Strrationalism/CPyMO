#include <cpymo_backend_text.h>
#include <cpymo_backend_masktrans.h>
#include <cpymo_engine.h>
#include <stb_truetype.h>
#include <stdlib.h>
#include <string.h>
#include <stb_image_write.h>

typedef struct {
    uint16_t w, h;
    float baseline;
    uint8_t px[0];
} cpymo_backend_text_impl;

extern cpymo_engine engine;
extern size_t window_size_w, window_size_h;
extern uint8_t *framebuffer;

extern stbtt_fontinfo font;
extern void cpymo_backend_font_render(void *out_or_null, int *w, int *h, cpymo_parser_stream_span text, float scale, float baseline);

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_parser_stream_span utf8_string, 
    float single_character_size_in_logical_screen)
{
    float height_norm = single_character_size_in_logical_screen / engine.gameconfig.imagesize_h;
    float height_screen = height_norm * window_size_h;

    float scale = stbtt_ScaleForPixelHeight(&font, height_screen);
    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, NULL, NULL);
    float baseline = scale * ascent;

    int w, h;
    cpymo_backend_font_render(NULL, &w, &h, utf8_string, scale, baseline);

    cpymo_backend_text_impl *o = (cpymo_backend_text_impl *)malloc(sizeof(cpymo_backend_text_impl) + w * h);
    if (o == NULL) return CPYMO_ERR_OUT_OF_MEM;
    memset(o, 0, sizeof(cpymo_backend_text_impl) + w * h);
    o->w = (uint16_t)w;
    o->h = (uint16_t)h;
    *out_width = cpymo_backend_text_width(utf8_string, single_character_size_in_logical_screen);
    cpymo_backend_font_render(o->px, &w, &h, utf8_string, scale, baseline);
    *out = o;

    scale = stbtt_ScaleForPixelHeight(&font, single_character_size_in_logical_screen);
    o->baseline = scale * ascent;

    return CPYMO_ERR_SUCC;   
}

void cpymo_backend_text_free(cpymo_backend_text t){ free(t); }

static void cpymo_backend_text_draw_internal(cpymo_color col, float x, float y, float alpha, cpymo_backend_text_impl *t)
{
    x /= engine.gameconfig.imagesize_w;
    x *= window_size_w;
    y /= engine.gameconfig.imagesize_h;
    y *= window_size_h;

    for (uint16_t draw_rect_y = 0; draw_rect_y < t->h; ++draw_rect_y) {
        for (uint16_t draw_rect_x = 0; draw_rect_x < t->w; ++draw_rect_x) {
            size_t draw_x = draw_rect_x + (size_t)x;
            size_t draw_y = draw_rect_y + (size_t)y;

            if (draw_x >= window_size_w || draw_y >= window_size_h) continue;

            float pixel_alpha = alpha * ((float)t->px[draw_rect_y * t->w + draw_rect_x] / 255.0f);

            float dst_r = framebuffer[draw_y * window_size_w * 3 + draw_x * 3 + 0] / 255.0f;
            float dst_g = framebuffer[draw_y * window_size_w * 3 + draw_x * 3 + 1] / 255.0f;
            float dst_b = framebuffer[draw_y * window_size_w * 3 + draw_x * 3 + 2] / 255.0f;

            float src_r = col.r / 255.0f;
            float src_g = col.g / 255.0f;
            float src_b = col.b / 255.0f;

            float r = dst_r + (src_r - dst_r) * pixel_alpha;
            float g = dst_g + (src_g - dst_g) * pixel_alpha;
            float b = dst_b + (src_b - dst_b) * pixel_alpha;

            framebuffer[draw_y * window_size_w * 3 + draw_x * 3 + 0] = (uint8_t)(r * 255.0f);
            framebuffer[draw_y * window_size_w * 3 + draw_x * 3 + 1] = (uint8_t)(g * 255.0f);
            framebuffer[draw_y * window_size_w * 3 + draw_x * 3 + 2] = (uint8_t)(b * 255.0f);
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

    cpymo_backend_text_draw_internal(cpymo_color_inv(col), x + 1, y + 1, alpha, t);
    cpymo_backend_text_draw_internal(col, x, y, alpha, t);
}

float cpymo_backend_text_width(
    cpymo_parser_stream_span s,
    float single_character_size_in_logical_screen)
{ 
    float height_norm = single_character_size_in_logical_screen / engine.gameconfig.imagesize_h;
    float height_screen = height_norm * window_size_h;
    float scale = stbtt_ScaleForPixelHeight(&font, height_screen);
    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, NULL, NULL);
    float baseline = scale * ascent;
    int w, h;
    cpymo_backend_font_render(NULL, &w, &h, s, scale, baseline);

    
    return (float)w / window_size_w * engine.gameconfig.imagesize_w;
}

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
    return CPYMO_ERR_UNSUPPORTED;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m){}

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans m, float t, bool is_fade_in){}
