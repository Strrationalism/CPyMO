#ifndef INCLUDE_CPYMO_BACKEND_IMAGE
#define INCLUDE_CPYMO_BACKEND_IMAGE

#include "../../cpymo/cpymo_error.h"
#include "../../cpymo/cpymo_color.h"
#include <stdbool.h>
#include <stddef.h>

typedef void *cpymo_backend_image;

enum cpymo_backend_image_format {
	cpymo_backend_image_format_rgb,
	cpymo_backend_image_format_rgba,
};

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, void *pixels_moveintoimage, int width, int height, enum cpymo_backend_image_format);

error_t cpymo_backend_image_load_with_mask(
	cpymo_backend_image *out_image, void *px_rgbx32_moveinto, void *mask_a8_moveinto, int w, int h, int mask_w, int mask_h);


void cpymo_backend_image_free(cpymo_backend_image image);


/* Screen Coord in backend renderer
 * 
 * LeftTop - (0, 0)
 * RightBottom - (gameconfig.imagesize.w - 1, gameconfig.imagesize.h - 1)
 */


enum cpymo_backend_image_draw_type {
	cpymo_backend_image_draw_type_bg,
	cpymo_backend_image_draw_type_chara,
	cpymo_backend_image_draw_type_ui_bg,
	cpymo_backend_image_draw_type_ui_element_bg,
	cpymo_backend_image_draw_type_ui_element,
	cpymo_backend_image_draw_type_text_say,
	cpymo_backend_image_draw_type_text_say_textbox,
	cpymo_backend_image_draw_type_text_text,
	cpymo_backend_image_draw_type_titledate_bg,
	cpymo_backend_image_draw_type_titledate_text,
	cpymo_backend_image_draw_type_sel_img,
	cpymo_backend_image_draw_type_effect
};


void cpymo_backend_image_draw(
	float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type);

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type);

bool cpymo_backend_image_album_ui_writable();

#endif
