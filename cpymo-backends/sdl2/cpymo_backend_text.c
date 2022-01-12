#include <cpymo_backend_text.h>
#include <stb_truetype.h>
#include <stdlib.h>
#include <cpymo_parser.h>
#include <cpymo_backend_image.h>
#include <memory.h>
#include <math.h>
#include <cpymo_utils.h>
#include <SDL_render.h>
#include <assert.h>

extern stbtt_fontinfo font;

typedef struct {
    float scale;
    int ascent;
    float baseline;
    int width, height;
    cpymo_backend_image img;
} cpymo_backend_text_internal;


static void cpymo_backend_text_render(void *out_or_null, int *w, int *h, cpymo_parser_stream_span text, float scale, float baseline) {
    float xpos = 0;

    int width = 0, height = 0, y_base = 0;
    while (text.len > 0) {
        uint32_t codepoint = cpymo_parser_stream_span_utf8_try_head_to_utf32(&text);
        int x0, y0, x1, y1;

        int advance_width, lsb;
        float x_shift = xpos - (float)floor(xpos);
        stbtt_GetCodepointHMetrics(&font, (int)codepoint, &advance_width, &lsb);

        if (codepoint == '\n') {
            xpos = 0;
            y_base += baseline;

            continue;
        }

        stbtt_GetCodepointBitmapBoxSubpixel(&font, (int)codepoint, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);
        if(out_or_null)
            stbtt_MakeCodepointBitmapSubpixel(
                &font, 
                (unsigned char *)out_or_null + (int)xpos + x0 + (int)(baseline + y0 + y_base) * *w, 
                x1 - x0, y1 - y0, *w, scale, scale, x_shift, 0, (int)codepoint);

        xpos += (advance_width * scale);

        cpymo_parser_stream_span text2 = text;
        uint32_t next_char = cpymo_parser_stream_span_utf8_try_head_to_utf32(&text2);
        if (next_char) {
            xpos += scale * stbtt_GetCodepointKernAdvance(&font, codepoint, next_char);
        }

        int new_width = (int)ceil(xpos);
        if (new_width > width) width = new_width;

        int new_height = (y1 - y0) + baseline + y_base;
        if (new_height > height) height = new_height;
    }

    *w = width;
    *h = height;
}


error_t cpymo_backend_text_create(
    cpymo_backend_text *out,
    cpymo_parser_stream_span utf8_string,
    float single_character_size_in_logical_screen)
{
    cpymo_parser_stream_span text = utf8_string;
    if (text.len == 0) return CPYMO_ERR_INVALID_ARG;

    cpymo_backend_text_internal *t = 
        (cpymo_backend_text_internal *)malloc(sizeof(cpymo_backend_text_internal));
    if (t == NULL) return CPYMO_ERR_OUT_OF_MEM;

    t->scale = stbtt_ScaleForPixelHeight(&font, single_character_size_in_logical_screen);
    stbtt_GetFontVMetrics(&font, &t->ascent, NULL, NULL);
    t->baseline = t->scale * t->ascent;

    cpymo_backend_text_render(NULL, &t->width, &t->height, text, t->scale, t->baseline);
    
    assert(t->width > 0 && t->height > 0);
    
    void *screen = malloc(t->width * t->height);
    if (screen == NULL) {
        free(t);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    memset(screen, 0, t->width * t->height);

    int w = t->width, h = t->height;
    cpymo_backend_text_render(screen, &w, &h, text, t->scale, t->baseline);

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
    cpymo_backend_text_internal *t = (cpymo_backend_text_internal *)text;

    SDL_SetTextureColorMod((SDL_Texture *)t->img, 255 - col.r, 255 - col.g, 255 - col.b);
    cpymo_backend_image_draw(
        x + 2,
        y_baseline - t->baseline + 2,
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
