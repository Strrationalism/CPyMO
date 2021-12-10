#ifndef INCLUDE_CPYMO_BACKEND_IMAGE
#define INCLUDE_CPYMO_BACKEND_IMAGE

#include "../../cpymo/cpymo_error.h"
#include "../../cpymo/cpymo_color.h"
#include <utf8.h>
#include <stdbool.h>

typedef void * cpymo_backend_image;

error_t cpymo_backend_image_load_immutable(
	cpymo_backend_image *out_image, void *pixels_moveintoimage, int width, int height, int channels);

error_t cpymo_backend_image_font(
	cpymo_backend_image *out_image, const char *utf8str, float size, cpymo_color color, bool aa);

error_t cpymo_backend_image_create_mutable(
	cpymo_backend_image *out_image, int width, int height);

error_t cpymo_backend_image_update_mutable(
	cpymo_backend_image image, void *pixels_moveintoimage);


void cpymo_backend_image_free(cpymo_backend_image image);


error_t cpymo_backend_image_size(cpymo_backend_image img, int *w, int *h);


/* Screen Coord in backend renderer
 * 
 * LeftTop - (0, 0)
 * RightBottom - (gameconfig.imagesize.w - 1, gameconfig.imagesize.h - 1)
 */


enum cpymo_backend_image_draw_type {
	cpymo_backend_image_draw_type_bg,
	cpymo_backend_image_draw_type_chara,
	cpymo_backend_image_draw_type_textbox,
	cpymo_backend_image_draw_type_uielement,
	cpymo_backend_image_draw_type_text_say,
	cpymo_backend_image_draw_type_text_text,
	cpymo_backend_image_draw_type_text_ui,
	cpymo_backend_image_draw_type_effect
};


void cpymo_backend_image_draw(
	float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type);

void cpymo_backend_image_draw_lines(
	const float *xyxy_points, size_t points_count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type);

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type);

#endif
