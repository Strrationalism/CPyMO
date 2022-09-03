#include <cpymo_prelude.h>
#include <cpymo_backend_software.h>
#include <cpymo_backend_masktrans.h>
#include <stddef.h>
#include <stdlib.h>

extern void cpymo_backend_image_scale_on_load(
    void **pixels, int *width, int *height, size_t channels);

error_t cpymo_backend_masktrans_create(
    cpymo_backend_masktrans *out,
    void *mask_singlechannel_moveinto,
    int w, int h)
{ 
    cpymo_backend_image_scale_on_load(
        &mask_singlechannel_moveinto,
        &w, &h, 1);
    
    cpymo_backend_software_image *img =
        (cpymo_backend_software_image *)malloc(sizeof(*img));
    if (img == NULL) return CPYMO_ERR_OUT_OF_MEM;

    img->r_offset = 0;
    img->g_offset = 0;
    img->b_offset = 0;
    img->a_offset = 0;
    img->w = w;
    img->h = h;
    img->has_alpha_channel = true;
    img->line_stride = w;
    img->pixel_stride = 1;
    img->pixels = (uint8_t *)mask_singlechannel_moveinto;
    
    *out = img;
    return CPYMO_ERR_SUCC;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m) 
{
    cpymo_backend_software_image *img =
        (cpymo_backend_software_image *)m;
    free(img->pixels);
    free(img);
}

void cpymo_backend_masktrans_draw(
    cpymo_backend_masktrans m, 
    float t, bool is_fade_in)
{
    extern cpymo_backend_software_context 
        *cpymo_backend_software_cur_context;
    
    cpymo_backend_software_image *render_target =
        cpymo_backend_software_cur_context->render_target;

    if (!is_fade_in) t = 1.0f - t;

    const float radius = 0.25f;
    float t_top = t + radius;
	float t_bottom = t - radius;

    for (size_t y = 0; y < render_target->h; ++y) {
        for (size_t x = 0; x < render_target->w; ++x) {
            float mask;
            float dummy;
            cpymo_backend_software_image_sample_nearest(
                (cpymo_backend_software_image *)m,
                (float)x / (float)render_target->w,
                (float)y / (float)render_target->h,
                &mask, &dummy, &dummy, &dummy);

            if (mask > t_top) mask = 1.0f;
			else if (mask < t_bottom) mask = 0.0f;
			else mask = (mask - t_bottom) / (2 * radius);

            cpymo_backend_software_image_write_blend(
                render_target, x, y, 0, 0, 0, mask);
        }
    }
}
