#include <stb_image.h>
#include <cpymo_error.h>
#include <cpymo_parser.h>

typedef struct {
	stbi_uc *pixels;
	size_t width, height, channels;
} cpymo_tool_image;

error_t cpymo_tool_image_load_from_file(cpymo_tool_image *out, const char *filename, bool load_mask);
error_t cpymo_tool_image_create(cpymo_tool_image *out, size_t w, size_t h, size_t channels);
void cpymo_tool_image_free(cpymo_tool_image out);

error_t cpymo_tool_image_create_mask(cpymo_tool_image *out_mask, const cpymo_tool_image *img);
error_t cpymo_tool_image_remove_alpha(cpymo_tool_image *out_rgb_img, const cpymo_tool_image *img);
error_t cpymo_tool_image_resize(cpymo_tool_image *out, const cpymo_tool_image *image, size_t w, size_t h);
error_t cpymo_tool_image_blit(cpymo_tool_image *dst, const cpymo_tool_image *src, size_t x, size_t y);

error_t cpymo_tool_image_save_to_file(
	const cpymo_tool_image *img, 
	const char *filename, 
	cpymo_parser_stream_span format);

error_t cpymo_tool_get_mask_name(char **out_mask_filename, const char *filename);
