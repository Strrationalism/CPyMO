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

#include <SDL.h>
#include <cpymo_engine.h>
#include <cpymo_error.h>

cpymo_engine engine;
SDL_Surface *framebuffer;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define SCREEN_BPP 32

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


int main(int argc, char **argv) 
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[Error] SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    const char *gamedir = ".";
    gamedir = ".";
    if (argc > 1) {
        gamedir = argv[1];
    }

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


    framebuffer = SDL_SetVideoMode(
        SCREEN_WIDTH, 
        SCREEN_HEIGHT, 
        SCREEN_BPP, 
        SCREEN_FLAGS);
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

