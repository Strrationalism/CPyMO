#include <cpymo_prelude.h>
#include <cpymo_backend_text.h>

#ifdef ENABLE_SDL_TTF
#include <cpymo_engine.h>
#include <cpymo_utils.h>
#include <SDL/SDL_ttf.h>
#include "../include/cpymo_backend_text.h"

static TTF_Font *font = NULL;
static int prev_ptsize = 16;
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
    SDL_Surface *sur, *shadow;
    int w, h;
    float scsils_size;
} cpymo_backend_text_internal;

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_str utf8_string, 
    float single_character_size_in_logical_screen)
{
    char *str = alloca(utf8_string.len + 1);
    cpymo_str_copy(str, utf8_string.len + 1, utf8_string);

    cpymo_backend_font_update_size(single_character_size_in_logical_screen);

    SDL_Color c;
    c.r = 255;
    c.g = 255;
    c.b = 255;

#if ENABLE_SDL_TTF == 1
    SDL_Color bg;
    bg.r = 0;
    bg.g = 0;
    bg.b = 0;
#endif

#if ENABLE_SDL_TTF == 1
    SDL_Surface *text = TTF_RenderUTF8_Shaded(font, str, c, bg);
#elif ENABLE_SDL_TTF == 2
    SDL_Surface *text = TTF_RenderUTF8_Blended(font, str, c);
#else
    SDL_Surface *text = TTF_RenderUTF8_Solid(font, str, c);
#endif

    SDL_Surface *text_optimized = SDL_DisplayFormatAlpha(text);
    if (text_optimized) {
        SDL_FreeSurface(text);
        text = text_optimized;
    }

    if (text == NULL) return CPYMO_ERR_UNKNOWN;
    int w = text->w, h = text->h;

    cpymo_backend_text_internal *out_text =
        (cpymo_backend_text_internal *)
        malloc(sizeof(cpymo_backend_text_internal));
    if (out == NULL) {
        SDL_FreeSurface(text);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    out_text->sur = text;
    out_text->w = w;
    out_text->h = h;
    out_text->scsils_size = single_character_size_in_logical_screen;

    out_text->shadow = NULL;
#if ENABLE_SDL_TTF == 2
    SDL_Color black;
    black.r = 0;
    black.g = 0;
    black.b = 0;
    out_text->shadow = TTF_RenderUTF8_Blended(font, str, black);
    if (out_text) {
        SDL_Surface *shadow_opt = SDL_DisplayFormatAlpha(out_text->shadow);
        SDL_FreeSurface(out_text->shadow);
        out_text->shadow = shadow_opt;
    }
#endif

    *out = out_text;
    *out_width = (float)w;

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text text)
{
    cpymo_backend_text_internal *t = (cpymo_backend_text_internal *)text;
    SDL_FreeSurface(t->sur);
    if (t->shadow) SDL_FreeSurface(t->shadow);
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

    if (t->shadow)
        cpymo_backend_image_draw(
            x + 1, y + 1, w, h, (SDL_Surface *)t->shadow, 
            0, 0, w, h, alpha, draw_type);    
    
    cpymo_backend_image_draw(
        x, y, w, h, (SDL_Surface *)t->sur, 0, 0, w, h, alpha, draw_type);
}

float cpymo_backend_text_width(
    cpymo_str s,
    float single_character_size_in_logical_screen)
{
    cpymo_backend_font_update_size(single_character_size_in_logical_screen);

    char *str = alloca(s.len + 1);
    cpymo_str_copy(str, s.len + 1, s);

    int w;

    if (TTF_SizeUTF8(font, str, &w, NULL) != 0) {
        return 
            cpymo_str_utf8_len(s) 
            * single_character_size_in_logical_screen;
    }

    return (float)w;
}
#endif
