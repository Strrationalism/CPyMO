#include <cpymo_backend_text.h>
#include <stb_truetype.h>
#include <stdlib.h>
#include <cpymo_parser.h>
#include <cpymo_backend_image.h>
#include <memory.h>
#include <math.h>
#include <cpymo_utils.h>
#include "cpymo_import_sdl2.h"
#include <assert.h>

extern stbtt_fontinfo font;

typedef struct {
    float scale;
    int ascent;
    float baseline;
    int width, height;
    cpymo_backend_image img;
} cpymo_backend_text_internal;


void cpymo_backend_font_render(void *out_or_null, int *w, int *h, cpymo_parser_stream_span text, float scale, float baseline);

error_t cpymo_backend_text_create(
    cpymo_backend_text *out,
    float *out_width,
    cpymo_parser_stream_span utf8_string,
    float single_character_size_in_logical_screen)
{
    cpymo_parser_stream_span text = utf8_string;
    if (text.len == 0) 
        return CPYMO_ERR_INVALID_ARG;

    cpymo_backend_text_internal *t = 
        (cpymo_backend_text_internal *)malloc(sizeof(cpymo_backend_text_internal));
    if (t == NULL) return CPYMO_ERR_OUT_OF_MEM;

    t->scale = stbtt_ScaleForPixelHeight(&font, single_character_size_in_logical_screen);
    stbtt_GetFontVMetrics(&font, &t->ascent, NULL, NULL);
    t->baseline = t->scale * t->ascent;

    cpymo_backend_font_render(NULL, &t->width, &t->height, text, t->scale, t->baseline);
    
    assert(t->width > 0 && t->height > 0);
    
    void *screen = malloc(t->width * t->height);
    if (screen == NULL) {
        free(t);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    memset(screen, 0, t->width * t->height);

    int w = t->width, h = t->height;
    cpymo_backend_font_render(screen, &w, &h, text, t->scale, t->baseline);

    assert(w == t->width && h == t->height);

    void *screen2 = malloc(t->width * t->height * 4);
    if (screen2 == NULL) {
        free(screen);
        free(t);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    memset(screen2, 255, t->width * t->height * 4);

    cpymo_utils_attach_mask_to_rgba(screen2, screen, t->width, t->height);
    free(screen);

    error_t err = cpymo_backend_image_load(&t->img, screen2, t->width, t->height, cpymo_backend_image_format_rgba);
    if (err != CPYMO_ERR_SUCC) {
        free(screen2);
        free(t);
        return CPYMO_ERR_UNKNOWN;
    }

    *out = t;
    *out_width = (float)w;

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text t)
{
    cpymo_backend_text_internal *tt = (cpymo_backend_text_internal *)t;
    cpymo_backend_image_free(tt->img);
    free(t);
}

void cpymo_backend_text_draw(
    cpymo_backend_text text,   
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type)
{
#ifdef RENDER_LOGICAL_SIZE_UNSUPPORTED_FORCED_CENTERED
    extern void cpymo_backend_image_calc_force_center_offset(float *posx, float *posy);
    cpymo_backend_image_calc_force_center_offset(&x, &y_baseline);
#endif

    cpymo_backend_text_internal *t = (cpymo_backend_text_internal *)text;

    SDL_SetTextureColorMod((SDL_Texture *)t->img, 255 - col.r, 255 - col.g, 255 - col.b);
    cpymo_backend_image_draw(
        x + 1,
        y_baseline - t->baseline + 1,
        (float)t->width,
        (float)t->height,
        t->img,
        0,
        0,
        t->width,
        t->height,
        alpha,
        draw_type);

    SDL_SetTextureColorMod((SDL_Texture *)t->img, col.r, col.g, col.b);
    cpymo_backend_image_draw(
        x,
        y_baseline - t->baseline,
        (float)t->width,
        (float)t->height,
        t->img,
        0,
        0,
        t->width,
        t->height,
        alpha,
        draw_type);
}

float cpymo_backend_text_width(cpymo_parser_stream_span t, float single_character_size_in_logical_screen)
{
    float scale = stbtt_ScaleForPixelHeight(&font, single_character_size_in_logical_screen);
    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, NULL, NULL);
    float baseline = scale * ascent;

    int w, h;
    cpymo_backend_font_render(NULL, &w, &h, t, scale, baseline);

    return (float)w;
}

#ifdef ENABLE_TEXT_EXTRACT
void cpymo_backend_text_extract(const char *text)
{
#ifdef ENABLE_TEXT_EXTRACT_ANDROID_ACCESSABLE
    extern void cpymo_android_text_to_speech(const char *text);

    cpymo_android_text_to_speech(text);
#endif

#ifdef ENABLE_TEXT_EXTRACT_COPY_TO_CLIPBOARD
    SDL_SetClipboardText(text);
#endif
}
#endif