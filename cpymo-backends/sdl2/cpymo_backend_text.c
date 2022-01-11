#include <cpymo_backend_text.h>
#include <stb_truetype.h>
#include <stdlib.h>
#include <cpymo_parser.h>
#include <cpymo_backend_image.h>
#include <memory.h>
#include <math.h>
#include <cpymo_utils.h>
#include <SDL_render.h>

extern stbtt_fontinfo font;

struct cpymo_backend_text {
    float scale;
    int ascent;
    float baseline;
    int width, height;
    cpymo_backend_image img;
};

error_t cpymo_backend_text_create(
    cpymo_backend_text *out,
    const char *utf8_string,
    float single_character_size_in_logical_screen)
{
    struct cpymo_backend_text *t = (struct cpymo_backend_text *)malloc(sizeof(struct cpymo_backend_text));
    if (t == NULL) return CPYMO_ERR_OUT_OF_MEM;

    t->width = 1024;
    t->height = 768;
    void *screen = malloc(1024 * 768);
    if (screen == NULL) {
        free(t);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    void *screen2 = malloc(1024 * 768 * 4);
    if (screen2 == NULL) {
        free(screen);
        free(t);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    memset(screen, 0, 1024 * 768);
    memset(screen2, 255, 1024 * 768 * 4);

    t->scale = stbtt_ScaleForPixelHeight(&font, single_character_size_in_logical_screen);
    stbtt_GetFontVMetrics(&font, &t->ascent, NULL, NULL);
    t->baseline = t->scale * t->ascent;

    float xpos = 0;

    cpymo_parser_stream_span span = cpymo_parser_stream_span_pure(utf8_string);
    while (span.len > 0) {
        uint32_t codepoint = cpymo_parser_stream_span_utf8_try_head_to_utf32(&span);
        int x0, y0, x1, y1;

        int advance_width, lsb;
        float x_shift = xpos - (float)floor(xpos);
        stbtt_GetCodepointHMetrics(&font, (int)codepoint, &advance_width, &lsb);
        stbtt_GetCodepointBitmapBoxSubpixel(&font, (int)codepoint, t->scale, t->scale, x_shift, 0, &x0, &y0, &x1, &y1);
        stbtt_MakeCodepointBitmapSubpixel(&font, (char *)screen + (int)xpos + x0 + (int)(t->baseline + y0) * 1024, x1 - x0, y1 - y0, 1024, t->scale, t->scale, x_shift, 0, (int)codepoint);
        xpos += (advance_width * t->scale);

        cpymo_parser_stream_span span2 = span;
        uint32_t next_char;
        if (next_char = cpymo_parser_stream_span_utf8_try_head_to_utf32(&span2)) {
            xpos += t->scale * stbtt_GetCodepointKernAdvance(&font, codepoint, next_char);
        }
    }

    cpymo_utils_attach_mask_to_rgba(screen2, screen, 1024, 768);
    free(screen);

    error_t err = cpymo_backend_image_load(&t->img, screen2, 1024, 768, cpymo_backend_image_format_rgba);
    if (err != CPYMO_ERR_SUCC) {
        free(t);
        return CPYMO_ERR_UNKNOWN;
    }

    *out = t;

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text t)
{
    struct cpymo_backend_text *tt = (struct cpymo_backend_text *)t;
    cpymo_backend_image_free(tt->img);
    free(t);
}

void cpymo_backend_text_draw(
    cpymo_backend_text text,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type)
{
    struct cpymo_backend_text *t = (struct cpymo_backend_text *)text;

    SDL_SetTextureColorMod((SDL_Texture *)t->img, 0, 0, 0);
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
