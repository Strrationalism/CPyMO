#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#define FASTEST_FILTER STBIR_FILTER_BOX
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE  FASTEST_FILTER
#define STBIR_DEFAULT_FILTER_UPSAMPLE    FASTEST_FILTER

#ifdef LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#endif

#include "cpymo_prelude.h"
#include <stdio.h>
#include <cpymo_engine.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <time.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
static uint64_t millis()
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    return st.wMilliseconds + st.wSecond * 1000 + st.wMinute * 1000 * 60 + st.wHour * 1000 * 60 * 60;
}

#else

static uint64_t millis()
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((uint64_t) now.tv_sec) * 1000 + ((uint64_t) now.tv_nsec) / 1000000;
}

#endif

static uint64_t prev;
static float get_delta_time() {
    uint64_t now = millis();
    float delta = (now - prev) / 1000.0f;
    prev = now;
    return delta;
}

cpymo_engine engine;

int main(int argc, char **argv) 
{
    srand((unsigned)time(NULL));
    prev = millis();

    extern error_t cpymo_backend_image_subsys_init(void);
    extern void cpymo_backend_image_subsys_free(void);
    error_t err = cpymo_backend_image_subsys_init();
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_backend_image_subsys_init: %s.\n", cpymo_error_message(err));
        return -1;
    }

    const char *gamedir = ".";
    if (argc == 2) {
        gamedir = argv[1];
    }

    extern error_t cpymo_backend_font_init(const char *gamedir);
    extern void cpymo_backend_font_free(void);
    err = cpymo_backend_font_init(gamedir);
    if (err != CPYMO_ERR_SUCC) {
        cpymo_backend_image_subsys_free();
        printf("[Error] cpymo_backend_font_init: %s.\n", cpymo_error_message(err));
        return -1;
    }

    err = cpymo_engine_init(&engine, gamedir);
    if (err != CPYMO_ERR_SUCC) {
        cpymo_backend_font_free();
        cpymo_backend_image_subsys_free();
        printf("[Error] cpymo_engine_init: %s.\n", cpymo_error_message(err));
        return -1;
    }

    printf("\033]0;%s\007", engine.gameconfig.gametitle);

    while (1) {
        bool redraw = false;

        error_t err = cpymo_engine_update(&engine, get_delta_time(), &redraw);
        if (err == CPYMO_ERR_NO_MORE_CONTENT) break;
        else if (err != CPYMO_ERR_SUCC) {
            printf("[Error] cpymo_engine_update: %s.\n", cpymo_error_message(err));
            cpymo_engine_free(&engine);
            cpymo_backend_font_free();
            cpymo_backend_image_subsys_free();
            return -1;
        }

        if (redraw) {
            extern void cpymo_backend_image_subsys_clear_framebuffer(void);
            cpymo_backend_image_subsys_clear_framebuffer();
            cpymo_engine_draw(&engine);

            extern void cpymo_backend_image_subsys_submit_framebuffer(void);
            cpymo_backend_image_subsys_submit_framebuffer(); 
        }
        else {
            usleep(16000);
        }
    }

    cpymo_engine_free(&engine);
    cpymo_backend_font_free();
    cpymo_backend_image_subsys_free();

    #ifdef LEAKCHECK
    stb_leakcheck_dumpmem();
    #endif

    return 0;
}

