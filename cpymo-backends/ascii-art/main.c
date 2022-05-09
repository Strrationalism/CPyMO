#include <stdio.h>
#include <cpymo_engine.h>

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

#include <time.h>
#include <unistd.h>

cpymo_engine engine;

int main(int argc, char **argv) 
{
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

    clock_t prev_time = clock();

    while (1) {
        bool redraw = false;

        clock_t cur_time = clock();
        float dur = (float)(cur_time - prev_time) / (float)CLOCKS_PER_SEC;
        error_t err = cpymo_engine_update(&engine, dur, &redraw);
        if (err != CPYMO_ERR_SUCC) {
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
            usleep(16);
        }

        prev_time = cur_time;
    }

    cpymo_engine_free(&engine);
    cpymo_backend_font_free();
    cpymo_backend_image_subsys_free();

    return 0;
}
