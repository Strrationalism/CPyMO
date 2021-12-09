#ifndef INCLUDE_CPYMO_BACKEND_IMAGE
#define INCLUDE_CPYMO_BACKEND_IMAGE

#include "../../cpymo/cpymo_error.h"
#include "../../cpymo/cpymo_color.h"
#include <utf8.h>

struct cpymo_backend_image;

error_t cpymo_backend_image_load(
	cpymo_backend_image **out_image, const char *path);

error_t cpymo_backend_image_move(
	cpymo_backend_image **out_image, const void *pixels_moveintoimage, int width, int height, int channels);

error_t cpymo_backend_image_font(
	cpymo_backend_image **out_image, const char *utf8str);

error_t cpymo_backend_image_update(
	cpymo_backend_image *image, const void *pixels_moveintoimage);

error_t cpymo_backend_image_attach_transparent_mask(
	cpymo_backend_image *image, const void *pixels_moveintoimage);


void cpymo_backend_image_free(cpymo_backend_image *image);


int cpymo_backend_image_width(const cpymo_backend_image *img);
int cpymo_backend_image_height(const cpymo_backend_image *img);


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
	const cpymo_backend_image *src,
	int srcx, int srcy, int srcw, int srch, float alpha,
	cpymo_backend_image_draw_type draw_type);


void cpymo_backend_image_fillcolor(
	cpymo_color col,
	float alpha);


#endif
