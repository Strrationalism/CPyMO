#ifndef INCLUDE_CPYMO_TOOL_IMAGE
#define INCLUDE_CPYMO_TOOL_IMAGE

#include "../stb/stb_image.h"
#include "../cpymo/cpymo_error.h"
#include "../cpymo/cpymo_parser.h"

typedef struct {
	stbi_uc *pixels;
	size_t width, height, channels;
} cpymo_tool_image;

error_t cpymo_tool_image_load_from_file(
    cpymo_tool_image *out, const char *filename,
    bool load_mask, const char *mask_format);

error_t cpymo_tool_image_create(cpymo_tool_image *out, size_t w, size_t h, size_t channels);
void cpymo_tool_image_free(cpymo_tool_image img);

void cpymo_tool_image_fill(cpymo_tool_image *img, uint8_t val);

error_t cpymo_tool_image_resize(cpymo_tool_image *out, const cpymo_tool_image *image, size_t w, size_t h);
void cpymo_tool_image_blit(cpymo_tool_image *dst, const cpymo_tool_image *src, int x, int y);

error_t cpymo_tool_image_save_to_file_with_mask(
    const cpymo_tool_image * img,
    const char * filename,
    const char * format,
    bool create_mask,
    const char *mask_format);

error_t cpymo_tool_get_mask_name(
	char **out_mask_filename,
	const char *filename,
	const char *maskext);

#endif

