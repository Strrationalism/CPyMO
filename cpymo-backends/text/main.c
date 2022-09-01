#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#ifdef LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#endif

#include <cpymo_prelude.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <cpymo_backend_image.h>
#include <cpymo_backend_masktrans.h>
#include <cpymo_backend_text.h>
#include <cpymo_engine.h>

cpymo_engine engine;

error_t cpymo_assetloader_load_image_with_mask(
	cpymo_backend_image *img, int *w, int *h, 
	cpymo_str name, 
	const char *asset_type,
	const char *asset_ext,
	const char *mask_ext,
	bool use_pkg,
	const cpymo_package *pkg,
	const cpymo_assetloader *loader,
	bool load_mask)
{
    *w = engine.gameconfig.imagesize_w;
    *h = engine.gameconfig.imagesize_h;
    *img = (cpymo_backend_image)1;
    return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_load_bg_image(
	cpymo_backend_image * img, int * w, int * h, 
	cpymo_str name, const cpymo_assetloader * loader)
{
    *w = engine.gameconfig.imagesize_w;
    *h = engine.gameconfig.imagesize_h;
    *img = (cpymo_backend_image)1;
    return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_load_system_masktrans(
	cpymo_backend_masktrans *out, cpymo_str name, 
	const cpymo_assetloader *loader)
{
    *out = (cpymo_backend_masktrans)1;
    return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_load_icon(
	cpymo_backend_image *out, int *w, int *h, const char *gamedir)
{
    *w = engine.gameconfig.imagesize_w;
    *h = engine.gameconfig.imagesize_h;
    *out = (cpymo_backend_image)1;
    return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_load(cpymo_backend_image *out_image, void *pixels_moveintoimage, int width, int height, enum cpymo_backend_image_format f)
{
    free(pixels_moveintoimage);
    *out_image = (cpymo_backend_image)1;
    return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_load_with_mask(cpymo_backend_image *out_image, void *px_rgbx32_moveinto, void *mask_a8_moveinto, int w, int h, int mask_w, int mask_h)
{
    free(px_rgbx32_moveinto);
    free(mask_a8_moveinto);
    *out_image = (cpymo_backend_image)1;
    return CPYMO_ERR_SUCC;
}

void cpymo_backend_image_free(cpymo_backend_image image){}

void cpymo_backend_image_draw(
    float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type
){}

void cpymo_backend_image_fill_rects(
    const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type
){}

bool cpymo_backend_image_album_ui_writable() { return false; }

error_t cpymo_backend_masktrans_create(
    cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{ return CPYMO_ERR_UNSUPPORTED; }

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m) {}

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans m, float t, bool is_fade_in) {}

#include "../sdl2/cpymo_backend_save.c"
#include "../ascii-art/cpymo_backend_input.c"

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_str utf8_string, 
    float single_character_size_in_logical_screen)
{
    *out = (cpymo_backend_text)1;
    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text t) {}

void cpymo_backend_text_draw(
    cpymo_backend_text t,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type) {}

float cpymo_backend_text_width(
    cpymo_str t,
    float single_character_size_in_logical_screen) 
{ return t.len * single_character_size_in_logical_screen; }

void cpymo_backend_text_extract(const char *text)
{ puts(text); }

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

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

static uint64_t prev;
static float get_delta_time() {
    uint64_t now = millis();
    float delta = (now - prev) / 1000.0f;
    prev = now;
    return delta;
}

int main(int argc, char **argv)
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
    #endif

    srand((unsigned)time(NULL));
    const char *gamedir = ".";
    if (argc == 2) {
        gamedir = argv[1];
    }

    error_t err = cpymo_engine_init(&engine, gamedir);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_engine_init: %s.\n", cpymo_error_message(err));
        return -1;
    }

    printf("\033]0;%s\007", engine.gameconfig.gametitle);

    prev = millis();

    while(1) {
        bool redraw = false;
        err = cpymo_engine_update(&engine, get_delta_time(), &redraw);
        if (err == CPYMO_ERR_NO_MORE_CONTENT) break;
        else if (err != CPYMO_ERR_SUCC) {
            printf("[Error] cpymo_engine_update: %s.\n", cpymo_error_message(err));
            cpymo_engine_free(&engine);
            return -1;
        }

        usleep(16000);
    }

    cpymo_engine_free(&engine);

    #ifdef LEAKCHECK
    stb_leakcheck_dumpmem();
    #endif

    return 0;
}
