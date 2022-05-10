#include <stdio.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define FASTEST_FILTER STBIR_FILTER_BOX
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE  FASTEST_FILTER
#define STBIR_DEFAULT_FILTER_UPSAMPLE    FASTEST_FILTER
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <SDL/SDL.h>
#include <cpymo_engine.h>
#include <cpymo_error.h>

cpymo_engine engine;
SDL_Surface *framebuffer;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define SCREEN_BPP 24

#ifndef SCREEN_BPP
#define SCREEN_BPP 0
#endif

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH engine.gameconfig.imagesize_w
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT engine.gameconfig.imagesize_h
#endif

#ifndef SCREEN_FLAGS
#define SCREEN_FLAGS (SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_ANYFORMAT | SDL_HWPALETTE | SDL_DOUBLEBUF)
#endif

static bool current_full_screen;
static error_t set_video_mode(size_t w, size_t h, bool fullscreen) 
{
    Uint32 flags = SCREEN_FLAGS;
    if (fullscreen)
        flags |= SDL_FULLSCREEN;

#ifdef SCREEN_RESIZABLE
    flags |= SDL_RESIZABLE;
#endif

    current_full_screen = fullscreen;

    framebuffer = SDL_SetVideoMode((int)w, (int)h, SCREEN_BPP, flags);
    if (framebuffer == NULL)
        return CPYMO_ERR_UNKNOWN;
    
    SDL_Rect clip;
    clip.x = (w - engine.gameconfig.imagesize_w) / 2;
    clip.y = (h - engine.gameconfig.imagesize_h) / 2;
    clip.w = engine.gameconfig.imagesize_w;
    clip.h = engine.gameconfig.imagesize_h;

    SDL_SetClipRect(framebuffer, &clip);
    return CPYMO_ERR_SUCC;
}

#ifndef LOAD_GAME_ICON
#define load_game_icon(...)
#else 
static inline void load_game_icon(const char *gamedir) 
{
    char *path = alloca(strlen(gamedir) + 10);
    sprintf(path, "%s/icon.png", gamedir);

    int w, h;
    stbi_uc *data = stbi_load(path, &w, &h, NULL, 4);
    if (data == NULL) return;

    void *px = (void *)malloc(32 * 32 * 4);
    if (px == NULL) {
        free(data);
        return;
    }

    stbir_resize_uint8(data, w, h, 0, px, 32, 32, 0, 4);
    free(data);

    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

    SDL_Surface *sur = SDL_CreateRGBSurfaceFrom(
		px,
		32,
		32,
		32,
		4 * 32,
		rmask,
		gmask,
		bmask,
		amask);

    if (sur == NULL) {
        free(px);
        return;
    }

    SDL_WM_SetIcon(sur, NULL);
    SDL_FreeSurface(sur);
    free(px);
}
#endif

int main(int argc, char **argv) 
{
    if (SDL_Init(
            SDL_INIT_VIDEO
#ifndef DISABLE_AUDIO
            | SDL_INIT_AUDIO
#endif
        ) < 0) {
        printf("[Error] SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    
    const char *gamedir = ".";
    

    if (argc > 1) {
        gamedir = argv[1];
    }

    load_game_icon(gamedir);

    error_t err = cpymo_engine_init(&engine, gamedir);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_engine_init: %s\n", cpymo_error_message(err));
        SDL_Quit();
        return -1;
    }

    extern error_t cpymo_backend_font_init(const char *gamedir);
    extern void cpymo_backend_font_free();
    err = cpymo_backend_font_init(gamedir);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_backend_font_init: %s\n", cpymo_error_message(err));
        cpymo_engine_free(&engine);
        SDL_Quit();
        return -1;
    }

    err = set_video_mode(SCREEN_WIDTH, SCREEN_HEIGHT, 
#ifdef DEFAULT_FULLSCREEN
    true
#else
    false
#endif
    );

    if (framebuffer == NULL) {
        printf("[Error] SDL_SetVideoMode: %s\n", SDL_GetError());
        cpymo_engine_free(&engine);
        cpymo_backend_font_free();
        SDL_Quit();
        return -1;
    }

    SDL_WM_SetCaption(
        engine.gameconfig.gametitle,
        engine.gameconfig.gametitle);

    Uint32 prev_time = SDL_GetTicks();
    int ret = 0;

    while (1) {
        bool redraw_system = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: goto EXIT;
            case SDL_VIDEOEXPOSE: redraw_system = true; break;
            case SDL_VIDEORESIZE:
                err = set_video_mode(event.resize.w, event.resize.h, current_full_screen);
                if (err != CPYMO_ERR_SUCC) {
                    printf("[Error] SDL_SetVideoMode: %s\n", SDL_GetError());
                    ret = -1;
                    goto EXIT;
                }
                redraw_system = true;
                break;
#ifdef TOGGLE_FULLSCREEN
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT)) {
                    err = set_video_mode(SCREEN_WIDTH, SCREEN_HEIGHT, !current_full_screen);
                    if (err != CPYMO_ERR_SUCC) {
                        printf("[Error] SDL_SetVideoMode: %s\n", SDL_GetError());
                        ret = -1;
                        goto EXIT;
                    }
                }
                break;
#endif
            };
        }

        bool redraw = false;
        Uint32 cur_time = SDL_GetTicks();
        err = cpymo_engine_update(
            &engine, 
            (float)(cur_time - prev_time) / 1000.0f, 
            &redraw);

        if (err != CPYMO_ERR_SUCC) {
            if (err == CPYMO_ERR_NO_MORE_CONTENT) goto EXIT;
            printf("[Error] cpymo_engine_update: %s\n", cpymo_error_message(err));
            ret = -1;
            goto EXIT;
        }

        if (redraw || redraw_system) {
            SDL_FillRect(framebuffer, NULL, 0);
            cpymo_engine_draw(&engine);
            SDL_Flip(framebuffer);
        }
        else {
            SDL_Delay(16);
        }

        prev_time = cur_time;
    }

EXIT:
    cpymo_engine_free(&engine);
    cpymo_backend_font_free();
    SDL_Quit();
    return ret;
}

