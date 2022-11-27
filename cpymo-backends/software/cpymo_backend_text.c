#include <cpymo_prelude.h>
#include <cpymo_error.h>
#include <cpymo_color.h>
#include <cpymo_str.h>
#include <cpymo_backend_software.h>
#include <cpymo_backend_text.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef TEXT_LINE_Y_OFFSET
#define TEXT_LINE_Y_OFFSET 0
#endif

#ifndef TEXT_CHARACTER_W_SCALE
#define TEXT_CHARACTER_W_SCALE 4
#endif

extern cpymo_backend_software_context 
    *cpymo_backend_software_cur_context;

static void cpymo_backend_text_render(
    void *out_or_null, 
    int *w, int *h, 
    cpymo_str text, 
    float scale, float baseline) 
{
    stbtt_fontinfo *font = cpymo_backend_software_cur_context->font;
	float xpos = 0;

	int width = 0, height = 0;
	float y_base = 0;
	while (text.len > 0) {
		uint32_t codepoint = cpymo_str_utf8_try_head_to_utf32(&text);
		int x0, y0, x1, y1;

		int advance_width, lsb;
		float x_shift = xpos - (float)floor(xpos);
		stbtt_GetCodepointHMetrics(font, (int)codepoint, &advance_width, &lsb);

		if (codepoint == '\n') {
			xpos = 0;

			y_base += baseline + TEXT_LINE_Y_OFFSET;

			continue;
		}

		stbtt_GetCodepointBitmapBoxSubpixel(font, (int)codepoint, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);
		if (out_or_null)
			stbtt_MakeCodepointBitmapSubpixel(
				font,
				(unsigned char *)out_or_null + (int)xpos + x0 + (int)(baseline + y0 + y_base) * *w,
				x1 - x0, y1 - y0, *w, scale, scale, x_shift, 0, (int)codepoint);

		xpos += (advance_width * scale);

		cpymo_str text2 = text;
		uint32_t next_char = cpymo_str_utf8_try_head_to_utf32(&text2);
		if (next_char) {
			xpos += scale * stbtt_GetCodepointKernAdvance(font, codepoint, next_char);
		}

		int new_width = (int)ceil(xpos);
		if (new_width > width) width = new_width;

		int new_height = (int)((y1 - y0) + baseline + y_base);
		if (new_height > height) height = new_height;
	}

	*w = width;
	*h = height;
}

typedef struct {
    size_t w, h;
    float baseline;
    uint8_t px[0];
} cpymo_backend_text_impl;

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_str utf8_string, 
    float single_character_size_in_logical_screen)
{
    float game_w = cpymo_backend_software_cur_context->logical_screen_w;
    float game_h = cpymo_backend_software_cur_context->logical_screen_h;

    float win_w = (float)cpymo_backend_software_cur_context->render_target->w;
    float win_h = (float)cpymo_backend_software_cur_context->render_target->h;

    stbtt_fontinfo *font = cpymo_backend_software_cur_context->font;

    float height_norm = single_character_size_in_logical_screen / game_h;
    float height_screen = height_norm * win_h;

    float scale = stbtt_ScaleForPixelHeight(font, height_screen);
    int ascent;
    stbtt_GetFontVMetrics(font, &ascent, NULL, NULL);
    float baseline = scale * ascent;

    int w, h;
    cpymo_backend_text_render(NULL, &w, &h, utf8_string, scale, baseline);
    h += 4; // magic

    cpymo_backend_text_impl *o = (cpymo_backend_text_impl *)malloc(sizeof(cpymo_backend_text_impl) + w * h);
    if (o == NULL) return CPYMO_ERR_OUT_OF_MEM;
    memset(o, 0, sizeof(cpymo_backend_text_impl) + w * h);
    o->w = (uint16_t)w;
    o->h = (uint16_t)h;
    *out_width = cpymo_backend_text_width(utf8_string, single_character_size_in_logical_screen);
    cpymo_backend_text_render(o->px, &w, &h, utf8_string, scale, baseline);
    *out = o;

    scale = stbtt_ScaleForPixelHeight(font, single_character_size_in_logical_screen);
    o->baseline = scale * ascent;

    return CPYMO_ERR_SUCC;   
}

void cpymo_backend_text_free(cpymo_backend_text t){ free(t); }

static void cpymo_backend_text_draw_internal(cpymo_color col, float x, float y, float alpha, cpymo_backend_text_impl *t)
{
    cpymo_backend_software_image *render_target = 
        cpymo_backend_software_cur_context->render_target;

    float 
        window_size_w = (float)render_target->w,
        window_size_h = (float)render_target->h;
    
    x /= cpymo_backend_software_cur_context->logical_screen_w;
    x *= window_size_w;
    y /= cpymo_backend_software_cur_context->logical_screen_h;
    y *= window_size_h;

    for (uint16_t draw_rect_y = 0; draw_rect_y < t->h; ++draw_rect_y) {
        for (uint16_t draw_rect_x = 0; draw_rect_x < t->w * TEXT_CHARACTER_W_SCALE; ++draw_rect_x) {
            size_t draw_x = draw_rect_x + (size_t)x;
            size_t draw_y = draw_rect_y + (size_t)y;

            if (draw_x >= window_size_w || draw_y >= window_size_h) continue;

            float pixel_alpha = 
                ((float)t->px[draw_rect_y * t->w + draw_rect_x / TEXT_CHARACTER_W_SCALE] / 255.0f);

            cpymo_backend_software_image_write_blend(
                render_target, 
                draw_x,
                draw_y,
                (float)col.r / 255.0f,
                (float)col.g / 255.0f,
                (float)col.b / 255.0f,
                alpha * pixel_alpha);
        }
    }
}

void cpymo_backend_text_draw(
    cpymo_backend_text t_,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type)
{
    cpymo_backend_text_impl *t = (cpymo_backend_text_impl *)t_;
    float y = y_baseline - t->baseline;

    cpymo_backend_text_draw_internal(cpymo_color_inv(col), x + 1, y + 1, alpha, t);
    cpymo_backend_text_draw_internal(col, x, y, alpha, t);
}

float cpymo_backend_text_width(
    cpymo_str s,
    float single_character_size_in_logical_screen)
{ 
    float game_w = cpymo_backend_software_cur_context->logical_screen_w;
    float game_h = cpymo_backend_software_cur_context->logical_screen_h;

    float win_w = (float)cpymo_backend_software_cur_context->render_target->w;
    float win_h = (float)cpymo_backend_software_cur_context->render_target->h;

    stbtt_fontinfo *font = cpymo_backend_software_cur_context->font;

    float height_norm = single_character_size_in_logical_screen / game_h;
    float height_screen = height_norm * win_h;
    float scale = stbtt_ScaleForPixelHeight(font, height_screen);
    int ascent;
    stbtt_GetFontVMetrics(font, &ascent, NULL, NULL);
    float baseline = scale * ascent;
    int w, h;
    cpymo_backend_text_render(NULL, &w, &h, s, scale, baseline);

    return TEXT_CHARACTER_W_SCALE * (float)w / win_w * game_w;
}
