#ifndef INCLUDE_CPYMO_BACKEND_IMAGE
#define INCLUDE_CPYMO_BACKEND_IMAGE

#include "../../cpymo/cpymo_error.h"
#include "../../cpymo/cpymo_color.h"
#include <utf8.h>

struct cpymo_backend_image;

error_t cpymo_backend_image_load(cpymo_backend_image **out_image, const char *path);
error_t cpymo_backend_image_move(cpymo_backend_image **out_image, const void *pixels, int width, int height, int channels);
error_t cpymo_backend_image_font(cpymo_backend_image **out_image, utf8_int32_t codepoint);
error_t cpymo_backend_image_update(cpymo_backend_image *image, const void *pixels);

void cpymo_backend_image_free(cpymo_backend_image *image);

int cpymo_backend_image_width(const cpymo_backend_image *img);
int cpymo_backend_image_height(const cpymo_backend_image *img);


/* Screen Coord in backend renderer
 * 
 * LeftTop - (0, 0)
 * RightBottom - (gameconfig.imagesize.w - 1, gameconfig.imagesize.h - 1)
 */

void cpymo_backend_image_render(
	const cpymo_backend_image *image,
	float dstx, float dsty, float dstw, float dsth,
	int srcx, int srcy, int srcw, int srch, float alpha);


void cpymo_backend_image_fillscreen(
	cpymo_color col,
	float alpha);


#endif
