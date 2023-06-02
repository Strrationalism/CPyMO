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

#include "../../cpymo/cpymo_prelude.h"
#include "../../cpymo/cpymo_engine.h"
#include "../software/cpymo_backend_software.h"
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../stb/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../../stb/stb_image_resize.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../stb/stb_image_write.h"

#define STB_DS_IMPLEMENTATION
#include "../../stb/stb_ds.h"

#include <time.h>
#include <unistd.h>

static cpymo_engine engine;
static cpymo_backend_software_image render_target;
static cpymo_backend_software_context context;

static error_t init_context(void)
{
    extern void get_winsize(size_t *w, size_t *h);

    get_winsize(&render_target.w, &render_target.h);    
    render_target.line_stride = render_target.w * 3;
    render_target.pixel_stride = 3;
    render_target.r_offset = 0;
    render_target.g_offset = 1;
    render_target.b_offset = 2;
    render_target.has_alpha_channel = false;
    render_target.pixels = 
        (uint8_t *)malloc(render_target.line_stride * render_target.h);
    
    if (render_target.pixels == NULL) return CPYMO_ERR_OUT_OF_MEM;
    
    context.logical_screen_w = (float)engine.gameconfig.imagesize_w;
    context.logical_screen_h = (float)engine.gameconfig.imagesize_h;

    context.scale_on_load_image = true;
    context.scale_on_load_image_w_ratio = 
        (float)render_target.w / context.logical_screen_w;
    context.scale_on_load_image_h_ratio = 
        (float)render_target.h / context.logical_screen_h;
    context.render_target = &render_target;

    extern stbtt_fontinfo font;
    context.font = &font;
    
    cpymo_backend_software_set_context(&context);
    
    return CPYMO_ERR_SUCC;
}

static void free_context(void)
{
    free(render_target.pixels);
    cpymo_backend_software_set_context(NULL);
}

#ifdef _WIN32
#include <direct.h>
#define mkdir(x, y) _mkdir(x)
#else
#include <sys/stat.h>
#endif

static void ensure_save_dir(const char *gamedir)
{
	char *save_dir = (char *)alloca(strlen(gamedir) + 8);
	sprintf(save_dir, "%s/save", gamedir);
	mkdir(save_dir, 0777);
}

void on_exit(void)
{
    puts("\033[?25h\033[?1049l");
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    const char *gamedir = ".";
    if (argc == 2) 
        gamedir = argv[1];
    
    error_t err = cpymo_engine_init(&engine, gamedir);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_engine_init: %s. \n", cpymo_error_message(err));
        return -1;
    }

    extern error_t cpymo_backend_font_init(const char *gamedir);
    extern void cpymo_backend_font_free();

    err = cpymo_backend_font_init(gamedir);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_backend_font_init: %s.\n", 
            cpymo_error_message(err));
        cpymo_engine_free(&engine);
        return -1;
    }

    ensure_save_dir(gamedir);

    printf("\033]0;%s\007", engine.gameconfig.gametitle);

    err = init_context();
    if (err != CPYMO_ERR_SUCC) {
        cpymo_engine_free(&engine);
        cpymo_backend_font_free();    
        printf("[Error] init_context: %s.\n", cpymo_error_message(err));
        return -1;
    }

    int ret = 0;

    atexit(&on_exit);
    puts("\033[?25l\033[?1049h");

    extern float get_delta_time(void);
    get_delta_time();

    while(1) {
        bool redraw = false;

        err = cpymo_engine_update(&engine, get_delta_time(), &redraw);
        if (err == CPYMO_ERR_NO_MORE_CONTENT) break;
        else if (err != CPYMO_ERR_SUCC) {
            printf("[Error] cpymo_engine_update: %s.\n", cpymo_error_message(err));
            ret = -1;
            break;
        }

        if (redraw) {
            memset(
                render_target.pixels, 
                0, 
                render_target.line_stride * render_target.h);

            cpymo_engine_draw(&engine);

            extern void cpymo_backend_ascii_submit_framebuffer(
                const cpymo_backend_software_image *framebuffer);
            cpymo_backend_ascii_submit_framebuffer(&render_target);
        }
        else {
            usleep(16000);
        }
    }

// cleaning
    cpymo_engine_free(&engine);
    free_context();
    cpymo_backend_font_free();

    extern void cpymo_backend_ascii_clean(void);
    cpymo_backend_ascii_clean();

    #ifdef LEAKCHECK
    stb_leakcheck_dumpmem();
    #endif

    return ret;
}
