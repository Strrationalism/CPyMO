#include <cpymo_backend_text.h>
#include <SDL.h>
#include <stb_truetype.h>
#include <cpymo_backend_image.h>
#include <cpymo_utils.h>
#include <cpymo_engine.h>

const extern stbtt_fontinfo font;
const extern cpymo_engine engine;
extern void cpymo_backend_font_render(
    void *out_or_null, int *w, int *h, 
    cpymo_parser_stream_span text, float scale, float baseline);

typedef struct {
    cpymo_backend_image img;
    float baseline;
} cpymo_backend_font_impl;

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_parser_stream_span utf8_string, 
    float single_character_size_in_logical_screen)
{
    float scale = 
        stbtt_ScaleForPixelHeight(&font, single_character_size_in_logical_screen);

    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, NULL, NULL);
    float baseline = scale * ascent;

    int w, h;
    cpymo_backend_font_render(NULL, &w, &h, utf8_string, scale, baseline);

    void *px = malloc(w * h);
    if (px == NULL) {
        return CPYMO_ERR_OUT_OF_MEM;
    }

    memset(px, 0, w * h);
    cpymo_backend_font_render(px, &w, &h, utf8_string, scale, baseline);

    void *px_rgba = malloc(w * h * 4);
    if (px_rgba == NULL) {
        free(px);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            uint8_t *px = 4 * y * w + 4 * x + (uint8_t *)px_rgba;
            px[0] = engine.gameconfig.textcolor.r;
            px[1] = engine.gameconfig.textcolor.g;
            px[2] = engine.gameconfig.textcolor.b;
        }
    }

    cpymo_backend_image img;
    error_t err = cpymo_backend_image_load_with_mask(
        &img, px_rgba, px, w, h, w, h);

    if(err != CPYMO_ERR_SUCC) {
        free(px);
        free(px_rgba);
        return err;
    }

    cpymo_backend_font_impl *impl = malloc(sizeof(cpymo_backend_font_impl));
    if (impl == NULL) {
        cpymo_backend_image_free(impl);
        return CPYMO_ERR_UNKNOWN;
    }

    impl->baseline = baseline;
    impl->img = img;

    *out = impl;
    *out_width = (float)w;
    
    return CPYMO_ERR_SUCC;   
}

void cpymo_backend_text_free(cpymo_backend_text t_)
{
    cpymo_backend_font_impl *t = (cpymo_backend_font_impl *)t_;
    cpymo_backend_image_free(t->img);
    free(t);
}

void cpymo_backend_text_draw(
    cpymo_backend_text t_,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type)
{
    cpymo_backend_font_impl *t = (cpymo_backend_font_impl *)t_;
    SDL_Surface *sur = (SDL_Surface *)t->img;
    cpymo_backend_image_draw(
        x, y_baseline - t->baseline,
        (float)sur->w, (float)sur->h,
        t->img, 0, 0, sur->w, sur->h,
        alpha, draw_type);
}

float cpymo_backend_text_width(
    cpymo_parser_stream_span s,
    float single_character_size_in_logical_screen)
{ 
    float scale = 
        stbtt_ScaleForPixelHeight(&font, single_character_size_in_logical_screen);

    int ascent;
    stbtt_GetFontVMetrics(&font, &ascent, NULL, NULL);
    float baseline = scale * ascent;

    int w, h;
    cpymo_backend_font_render(NULL, &w, &h, s, scale, baseline);

    return (float)w;
}

