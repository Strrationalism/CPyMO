#include "cpymo_prelude.h"
#ifdef ENABLE_SDL2_TTF
#include <cpymo_engine.h>
#include <cpymo_utils.h>
#include <SDL2/SDL_ttf.h>
#include "../include/cpymo_backend_text.h"

static TTF_Font *font = NULL;
static int prev_ptsize = 16;
extern SDL_Renderer * const renderer;
extern const cpymo_engine engine;

void cpymo_backend_font_free()
{
    if (font) {
        TTF_CloseFont(font);
        font = NULL;
    }
    
    TTF_Quit();
}

error_t cpymo_backend_font_init(const char *gamedir)
{
    if (TTF_Init() == -1) return CPYMO_ERR_UNKNOWN;

    const char *font_suffixes[] = {
        "/system/default.ttf",
        "/system/default.otf",
        "/system/default.fnt",
    };

    if (gamedir) {
        CPYMO_FOREACH_ARR(i, font_suffixes) {
            char *path = alloca(strlen(gamedir) + 2 + strlen(font_suffixes[i]));
            sprintf(path, "%s/%s", gamedir, font_suffixes[i]);
            font = TTF_OpenFont(path, prev_ptsize);
            if (font) break;
        }
    }

#ifdef GAME_SELECTOR_DIR
    if (font == NULL) {
        const char *font_paths[] = {
            GAME_SELECTOR_DIR "/default.ttf",
            GAME_SELECTOR_DIR "/default.otf",
            GAME_SELECTOR_DIR "/default.fnt",
        };

        CPYMO_FOREACH_ARR(i, font_paths) {
            font = TTF_OpenFont(font_paths[i], prev_ptsize);
            if (font) break;
        }
    }
#endif

    if (font == NULL) {
        TTF_Quit();
        return CPYMO_ERR_CAN_NOT_OPEN_FILE;
    }

    return CPYMO_ERR_SUCC;
}

static void cpymo_backend_font_update_size(float s)
{
    int ptsize = s;
    if (ptsize != prev_ptsize) {
        //TTF_SetFontSize(font, ptsize);
        //TTF_SetFontKerning(font, 1);
        prev_ptsize = ptsize;
        cpymo_backend_font_free();
        cpymo_backend_font_init(engine.assetloader.gamedir);
    }
}

typedef struct {
    SDL_Texture *tex;
    int w, h;
    float scsils_size;
} cpymo_backend_text_internal;

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_parser_stream_span utf8_string, 
    float single_character_size_in_logical_screen)
{
    char *str = alloca(utf8_string.len + 1);
    cpymo_parser_stream_span_copy(str, utf8_string.len + 1, utf8_string);

    cpymo_backend_font_update_size(single_character_size_in_logical_screen);

    SDL_Color c;
    c.r = 255;
    c.g = 255;
    c.b = 255;
    c.a = 255;

#if ENABLE_SDL2_TTF == 1 || ENABLE_SDL2_TTF == 3
    SDL_Color bg;
    c.r = 0;
    c.g = 0;
    c.b = 0;
    c.a = 0;
#endif

#if ENABLE_SDL2_TTF == 1
    SDL_Surface *text = TTF_RenderText_Shaded_Wrapped(font, str, c, bg, 0);
#elif ENABLE_SDL2_TTF == 2
    SDL_Surface *text = TTF_RenderUTF8_Blended_Wrapped(font, str, c, 0);
#elif ENABLE_SDL2_TTF == 3
    SDL_Surface *text = TTF_RenderUTF8_LCD_Wrapped(font, str, c, bg, 0);
#else
    SDL_Surface *text = TTF_RenderUTF8_Solid_Wrapped(font, str, c, 0);
#endif

    if (text == NULL) return CPYMO_ERR_UNKNOWN;

    SDL_Texture *text_tex = SDL_CreateTextureFromSurface(renderer, text);
    int w = text->w, h = text->h;

    SDL_FreeSurface(text);
    if (text_tex == NULL) return CPYMO_ERR_UNKNOWN;

    SDL_SetTextureBlendMode(text_tex, SDL_BLENDMODE_BLEND);

    cpymo_backend_text_internal *out_text =
        (cpymo_backend_text_internal *)
        malloc(sizeof(cpymo_backend_text_internal));
    if (out == NULL) {
        SDL_DestroyTexture(text_tex);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    out_text->tex = text_tex;
    out_text->w = w;
    out_text->h = h;
    out_text->scsils_size = single_character_size_in_logical_screen;

    *out = out_text;
    *out_width = (float)w;

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text text)
{
    cpymo_backend_text_internal *t = (cpymo_backend_text_internal *)text;
    SDL_DestroyTexture(t->tex);
    free(t);
}

void cpymo_backend_text_draw(
    cpymo_backend_text text,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type)
{
    cpymo_backend_text_internal *t = (cpymo_backend_text_internal *)text;
    int w = t->w, h = t->h;

    float y = y_baseline - t->scsils_size;
    
    SDL_SetTextureColorMod(t->tex, 255 - col.r, 255 - col.g, 255 - col.b);
    cpymo_backend_image_draw(
        x + 1, y + 1, w, h, t->tex, 0, 0, w, h, alpha, draw_type);

    SDL_SetTextureColorMod(t->tex, col.r, col.g, col.b);
    cpymo_backend_image_draw(
        x, y, w, h, t->tex, 0, 0, w, h, alpha, draw_type);
}

float cpymo_backend_text_width(
    cpymo_parser_stream_span s,
    float single_character_size_in_logical_screen)
{
    cpymo_backend_font_update_size(single_character_size_in_logical_screen);

    char *str = alloca(s.len + 1);
    cpymo_parser_stream_span_copy(str, s.len + 1, s);

    int w;

    if (TTF_SizeUTF8(font, str, &w, NULL) != 0) {
        return 
            cpymo_parser_stream_span_utf8_len(s) 
            * single_character_size_in_logical_screen;
    }

    return (float)w;
}
#endif
