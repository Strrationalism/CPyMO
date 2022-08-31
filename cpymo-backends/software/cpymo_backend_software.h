#ifndef INCLUDE_CPYMO_BACKEND_SOFTWARE
#define INCLUDE_CPYMO_BACKEND_SOFTWARE

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

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
} cpymo_backend_software_context;

void cpymo_backend_software_set_context(
    cpymo_backend_software_context *context);

#endif
