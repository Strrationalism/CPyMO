#include "cpymo_backend_software.h"
#include <cpymo_backend_image.h>
#include <stb_image_resize.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef CPYMO_BACKEND_IMAGE_SOFTWARE_IMAGE_SAMPLER
#define CPYMO_BACKEND_IMAGE_SOFTWARE_IMAGE_SAMPLER nearest
#endif

extern cpymo_backend_image_software_context 
    *cpymo_backend_image_software_cur_context;

static void cpymo_backend_image_software_scale_on_load(
    void **pixels, int *width, int *height, size_t channels)
{
    if (!cpymo_backend_image_software_cur_context->scale_on_load_image)
        return;

    int new_width = 
        *width 
        * cpymo_backend_image_software_cur_context->scale_on_load_image_w_ratio;
        
    int new_height =
        *height
        * cpymo_backend_image_software_cur_context->scale_on_load_image_h_ratio;
        
    uint8_t *new_pixel = (uint8_t *)malloc(new_height * new_width * channels);
    if (new_pixel == NULL) return;

    stbir_resize_uint8(
        (uint8_t)*pixels, *width, *height, *width * channels, 
        new_pixel, new_width, new_height, new_width * channels, channels);

    free(*pixels);
    *pixels = *new_pixel;
    *width = new_width;
    *height = new_height;
}

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, 
    void *pixels_moveintoimage, 
    int width, int height, 
    enum cpymo_backend_image_format format)
{
    cpymo_backend_image_software_image *img =
        (cpymo_backend_image_software_image*)malloc(sizeof(*img));
    if (img == NULL) return CPYMO_ERR_OUT_OF_MEM;

    switch (format) {
    case cpymo_backend_image_format_rgb:
        img->pixel_stride = 3;
        img->has_alpha_channel = false;
        break;
    case cpymo_backend_image_format_rgba:
        img->pixel_stride = 4;
        img->has_alpha_channel = true;
        break;
    default:
        printf("[Error] Can not load from format %d.\n", format);
        abort();
    }

    cpymo_backend_image_software_scale_on_load(
        &pixels_moveintoimage, &width, &height, img->pixel_stride);

    img->w = (size_t)width;
    img->h = (size_t)height;
    img->pixels = pixels_moveintoimage;
    img->r_offset = 0;
    img->g_offset = 1;
    img->b_offset = 2;
    img->a_offset = 3;

    img->line_stride = img->w * img->pixel_stride;
    *out_image = (cpymo_backend_image *)img;
    return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_load_with_mask(
	cpymo_backend_image *out_image, 
    void *px_rgbx32_moveinto, 
    void *mask_a8_moveinto, 
    int w, int h, 
    int mask_w, int mask_h)
{
    cpymo_utils_attach_mask_to_rgba_ex(
        px_rgbx32_moveinto, w, h, mask_a8_moveinto, mask_w, mask_h);

	error_t err = cpymo_backend_image_load(
        out_image, px_rgbx32_moveinto, w, h, cpymo_backend_image_format_rgba);

    if (err == CPYMO_ERR_SUCC) 
        free(mask_a8_moveinto);
    
    return err;
}

void cpymo_backend_image_free(cpymo_backend_image image)
{
    cpymo_backend_image_software_image *p = 
        (cpymo_backend_image_software_image *)image;
    free(p->pixels);
    free(p);
}

void cpymo_backend_image_draw(
	float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{ abort(); }

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{ abort(); }

bool cpymo_backend_image_album_ui_writable()
{ 
    #ifdef ENABLE_ALBUM_UI_WRITEABLE
    return true;
    #else
    return false;
    #endif
}
