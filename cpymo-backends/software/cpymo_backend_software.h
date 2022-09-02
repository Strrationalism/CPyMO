#ifndef INCLUDE_CPYMO_BACKEND_SOFTWARE
#define INCLUDE_CPYMO_BACKEND_SOFTWARE

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stb_truetype.h>

typedef struct {
    size_t w, h, line_stride, pixel_stride;
    size_t r_offset, g_offset, b_offset, a_offset;
    bool has_alpha_channel;
    uint8_t *pixels;
} cpymo_backend_software_image;

#define CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(PIMAGE, X, Y, CHANNEL) \
    ((Y) * (PIMAGE)->line_stride + (X) * (PIMAGE)->pixel_stride + \
    (PIMAGE)->CHANNEL##_offset + (PIMAGE)->pixels)

typedef struct {
    bool scale_on_load_image;
    float scale_on_load_image_w_ratio, scale_on_load_image_h_ratio;

    float logical_screen_w, logical_screen_h;

    cpymo_backend_software_image *render_target;
    // render target will not write to alpha channel.

    stbtt_fontinfo *font;
} cpymo_backend_software_context;

void cpymo_backend_software_set_context(
    cpymo_backend_software_context *context);

static inline void cpymo_backend_software_image_write_blend(
    cpymo_backend_software_image *render_target,
    size_t x, size_t y,
    float r, float g, float b, float a)
{
    uint8_t *dst_r = CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(render_target, x, y, r);
    uint8_t *dst_g = CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(render_target, x, y, g);
    uint8_t *dst_b = CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(render_target, x, y, b);

    *dst_r = (uint8_t)((r * a + (float)*dst_r / 255.0f * (1.0f - a)) * 255.0f);
    *dst_g = (uint8_t)((g * a + (float)*dst_g / 255.0f * (1.0f - a)) * 255.0f);
    *dst_b = (uint8_t)((b * a + (float)*dst_b / 255.0f * (1.0f - a)) * 255.0f);
}

static inline void cpymo_backend_software_image_sample_nearest(
    const cpymo_backend_software_image *img,
    float u, float v,
    float *r, float *g, float *b, float *a)
{
    float tex_w = (float)img->w;
    float tex_h = (float)img->h;

    size_t su = (size_t)(u * (tex_w - 1));
    size_t sv = (size_t)(v * (tex_h - 1)); 

    *r = *CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(img, su, sv, r);
    *r /= 255.0f;

    *g = *CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(img, su, sv, g);
    *g /= 255.0f;

    *b = *CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(img, su, sv, b);
    *b /= 255.0f;

    if (img->has_alpha_channel) {
        *a = *CPYMO_BACKEND_SOFTWARE_IMAGE_PIXEL(img, su, sv, a);
        *a /= 255.0f;
    }
    else {
        *a = 1.0f;
    }
}

#endif
