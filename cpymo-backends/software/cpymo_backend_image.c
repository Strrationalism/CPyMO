#include <cpymo_prelude.h>
#include "cpymo_backend_software.h"
#include <cpymo_backend_image.h>
#include <cpymo_utils.h>
#include <stb_image_resize.h>
#include <stdlib.h>
#include <stdio.h>

extern cpymo_backend_software_context 
    *cpymo_backend_software_cur_context;

static void cpymo_backend_image_scale_on_load(
    void **pixels, int *width, int *height, size_t channels)
{
    if (!cpymo_backend_software_cur_context->scale_on_load_image)
        return;

    int new_width = 
        *width 
        * cpymo_backend_software_cur_context->scale_on_load_image_w_ratio;
        
    int new_height =
        *height
        * cpymo_backend_software_cur_context->scale_on_load_image_h_ratio;
        
    uint8_t *new_pixel = (uint8_t *)malloc(new_height * new_width * channels);
    if (new_pixel == NULL) return;

    stbir_resize_uint8(
        (uint8_t *)*pixels, *width, *height, *width * channels, 
        new_pixel, new_width, new_height, new_width * channels, channels);

    free(*pixels);
    *pixels = (void *)new_pixel;
    *width = new_width;
    *height = new_height;
}

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, 
    void *pixels_moveintoimage, 
    int width, int height, 
    enum cpymo_backend_image_format format)
{
    cpymo_backend_software_image *img =
        (cpymo_backend_software_image*)malloc(sizeof(*img));
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

    cpymo_backend_image_scale_on_load(
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
    cpymo_backend_software_image *p = 
        (cpymo_backend_software_image *)image;
    free(p->pixels);
    free(p);
}

static inline void cpymo_backend_image_trans_pos(float *x, float *y)
{
    float game_w = cpymo_backend_software_cur_context->logical_screen_w;
	float game_h = cpymo_backend_software_cur_context->logical_screen_h;

	float scr_w = (float)cpymo_backend_software_cur_context->render_target->w;
	float scr_h = (float)cpymo_backend_software_cur_context->render_target->h;

	*x = *x / game_w * scr_w;
	*y = *y / game_h * scr_h;
}

void cpymo_backend_image_draw(
	float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{ 
    cpymo_backend_image_trans_pos(&dstx, &dsty);
    cpymo_backend_image_trans_pos(&dstw, &dsth);

    int render_target_w = 
        (int)cpymo_backend_software_cur_context->render_target->w;

    int render_target_h = 
        (int)cpymo_backend_software_cur_context->render_target->h;

    int x1 = (int)dstx;
    int y1 = (int)dsty;
    int x2 = (int)(dstw + dstx);
    int y2 = (int)(dsth + dsty);

    dstw = (float)(x2 - x1);
    dsth = (float)(y2 - y1);

    cpymo_backend_software_image *srci = (cpymo_backend_software_image *)src;
    float u_offset = (float)srcx / (float)srci->w;
    float v_offset = (float)srcy / (float)srci->h;
    float u_scale = (float)srcw / (float)srci->w;
    float v_scale = (float)srch / (float)srci->h;

    for (int draw_y = y1; draw_y < y2; ++draw_y) {
        for (int draw_x = x1; draw_x < x2; ++draw_x) {
            if (draw_x < 0) continue;
            if (draw_x >= render_target_w) break;
            if (draw_y < 0) break;
            if (draw_y >= render_target_h) return;

            float u = u_offset + u_scale * (float)(draw_x - x1) / dstw;
            float v = v_offset + v_scale * (float)(draw_y - y1) / dsth;

            u = cpymo_utils_clampf(u, 0.0f, 1.0f);
            v = cpymo_utils_clampf(v, 0.0f, 1.0f);

            float r, g, b, a;
            cpymo_backend_software_image_sample_nearest(
                srci, u, v, &r, &g, &b, &a);
            
            cpymo_backend_software_image_write_blend(
                cpymo_backend_software_cur_context->render_target,
                (size_t)draw_x, (size_t)draw_y,
                r, g, b, a);
        }
    }
}

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{ 
    float r = cpymo_utils_clampf((float)color.r / 255.0f, 0.0f, 1.0f);
    float g = cpymo_utils_clampf((float)color.g / 255.0f, 0.0f, 1.0f);
    float b = cpymo_utils_clampf((float)color.b / 255.0f, 0.0f, 1.0f);
    alpha = cpymo_utils_clampf(alpha, 0.0f, 1.0f);

    int render_target_w = 
        (int)cpymo_backend_software_cur_context->render_target->w;

    int render_target_h = 
        (int)cpymo_backend_software_cur_context->render_target->h;

    for (size_t i = 0; i < count; ++i) {
        const float *rect = xywh + 4 * i;
        float x = rect[0];
        float y = rect[1];
        float w = rect[2];
        float h = rect[3];
        cpymo_backend_image_trans_pos(&x, &y);
        cpymo_backend_image_trans_pos(&w, &h);

        int x1 = (int)x;
        int y1 = (int)y;
        int x2 = (int)(x + w);
        int y2 = (int)(y + h);

        for (int draw_y = y1; draw_y < y2; ++draw_y) {
            for (int draw_x = x1; draw_x < x2; ++draw_x) {
                if (draw_x < 0) continue;
                if (draw_y < 0) break;
                if (draw_x >= render_target_w) break;
                if (draw_y >= render_target_h) break;

                cpymo_backend_software_image_write_blend(
                    cpymo_backend_software_cur_context->render_target,
                    (size_t)draw_x,
                    (size_t)draw_y,
                    r, g, b, alpha);
            }
        }
    }
}

bool cpymo_backend_image_album_ui_writable()
{ 
    #ifdef ENABLE_ALBUM_UI_WRITEABLE
    return true;
    #else
    return false;
    #endif
}
