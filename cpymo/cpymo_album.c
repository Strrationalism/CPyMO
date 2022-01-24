#include <stb_image_resize.h>
#include <stb_image_write.h>
#include <stb_image.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include "cpymo_error.h"
#include "cpymo_parser.h"
#include <cpymo_backend_image.h>
#include "cpymo_assetloader.h"

#ifdef __3DS__
#define _itoa itoa
#endif

static error_t cpymo_album_generate_album_ui_image(
	cpymo_backend_image *out_image, 
	cpymo_parser_stream_span album_list_text, 
	cpymo_parser_stream_span output_cache_ui_file_name,
	size_t page, 
	cpymo_assetloader* loader,
	size_t *ref_w, size_t *ref_h)
{
	stbi_uc *pixels = NULL;

	{
		char *image_buf = NULL;
		size_t image_buf_size = 0;
		error_t err = cpymo_assetloader_load_system(&image_buf, &image_buf_size, "albumbg", "png", loader);
		if (err == CPYMO_ERR_SUCC) {
			int w2, h2, channels;
			pixels = stbi_load_from_memory((stbi_uc *)image_buf, (int)image_buf_size, &w2, &h2, &channels, 3);
			free(image_buf);

			if (pixels != NULL) {
				*ref_w = (size_t)w2;
				*ref_h = (size_t)h2;
			}
		}

		if (pixels == NULL) {
			pixels = (stbi_uc *)malloc(*ref_w * *ref_h * 3);
			if (pixels == NULL) return CPYMO_ERR_OUT_OF_MEM;
			memset(pixels, 0, *ref_w * *ref_h * 3);
		}
	}

	cpymo_parser album_list_parser;
	cpymo_parser_init(&album_list_parser, album_list_text.begin, album_list_text.len);

	size_t next_id = 0;

	const size_t thumb_width = (size_t)(0.17 * (double)*ref_w);
	const size_t thumb_height = (size_t)(0.17 * (double)*ref_h);

	do {
		cpymo_parser_stream_span page_str = cpymo_parser_curline_pop_commacell(&album_list_parser);
		cpymo_parser_stream_span_trim(&page_str);

		if (page_str.len <= 0) continue;
		if (cpymo_parser_stream_span_atoi(page_str) != (int)page + 1) continue;

		const size_t id = next_id++;

		cpymo_parser_stream_span thumb_count_str = 
			cpymo_parser_curline_pop_commacell(&album_list_parser);
		cpymo_parser_stream_span_trim(&thumb_count_str);
		if (thumb_count_str.len <= 0) continue;
		const int thumb_count = cpymo_parser_stream_span_atoi(thumb_count_str);

		if (thumb_count < 1) continue;

		cpymo_parser_curline_pop_commacell(&album_list_parser);	// discard CG name.

		const size_t col = id % 5;
		const size_t row = id / 5;
		const size_t thumb_left_top_x = (size_t)ceil((0.03 + 0.19 * col) * (double)*ref_w);
		const size_t thumb_left_top_y = (size_t)ceil((0.02 + 0.19 * row) * (double)*ref_h);
		const size_t thumb_right_bottom_x = thumb_left_top_x + thumb_width;
		const size_t thumb_right_bottom_y = thumb_left_top_y + thumb_height;

		if (thumb_right_bottom_x >= *ref_w || thumb_right_bottom_y >= *ref_h) continue;

		int cg_w, cg_h;
		stbi_uc *thumb_pixels = NULL;
		for (int i = 0; i < thumb_count; ++i) {
			// Try load ONE thumb.
			char bgname[64];
			cpymo_parser_stream_span bgname_span = 
				cpymo_parser_curline_pop_commacell(&album_list_parser);
			cpymo_parser_stream_span_trim(&bgname_span);
			if (bgname_span.len <= 0) continue;
			cpymo_parser_stream_span_copy(bgname, sizeof(bgname), bgname_span);

			char *buf = NULL;
			size_t buf_size = 0;
			error_t err = cpymo_assetloader_load_bg(&buf, &buf_size, bgname, loader);
			if (err != CPYMO_ERR_SUCC) continue;

			int cg_channels;
			thumb_pixels = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &cg_w, &cg_h, &cg_channels, 3);
			free(buf);

			if (thumb_pixels != NULL) break;
		}

		if (thumb_pixels == NULL) continue;

		stbir_resize_uint8(
			thumb_pixels, cg_w, cg_h, cg_w * 3, 
			pixels + 3 * thumb_left_top_y * *ref_w + 3 * thumb_left_top_x, 
			(int)thumb_width, (int)thumb_height, (int)*ref_w * 3, 3);
		free(thumb_pixels);
	} while (cpymo_parser_next_line(&album_list_parser));

	if (cpymo_backend_image_album_ui_writable()) {
		char *path = (char *)malloc(strlen(loader->gamedir) + output_cache_ui_file_name.len + 32);
		if (path != NULL) {
			strcpy(path, loader->gamedir);
			strcat(path, "/system/");
			strncat(path, output_cache_ui_file_name.begin, output_cache_ui_file_name.len);
			strcat(path, "_");
			char page_str[4];
			_itoa((int)page, page_str, 10);
			strcat(path, page_str);
			strcat(path, ".png");
			stbi_write_png(path, (int)*ref_w, (int)*ref_h, 3, pixels, (int)*ref_w * 3);
			free(path);
		}
	}

	error_t err = cpymo_backend_image_load(out_image, pixels, (int)*ref_w, (int)*ref_h, cpymo_backend_image_format_rgb);
	if (err != CPYMO_ERR_SUCC) {
		free(pixels);
		return err;
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_album_load_ui_image(
	cpymo_backend_image *out_image,
	cpymo_parser_stream_span album_list_text,
	cpymo_parser_stream_span output_cache_ui_file_name,
	size_t page,
	cpymo_assetloader *loader,
	size_t *ref_w, size_t *ref_h) 
{
	char *assetname = (char *)malloc(output_cache_ui_file_name.len + 16);
	if (assetname == NULL) return CPYMO_ERR_OUT_OF_MEM;

	cpymo_parser_stream_span_copy(assetname, output_cache_ui_file_name.len + 16, output_cache_ui_file_name);
	char page_str[4];
	_itoa((int)page, page_str, 10);
	strcat(assetname, "_");
	strcat(assetname, page_str);

	char *buf = NULL;
	size_t buf_size = 0;
	error_t err = cpymo_assetloader_load_system(&buf, &buf_size, assetname, "png", loader);
	free(assetname);

	if (err != CPYMO_ERR_SUCC) {
		GENERATE:
		return cpymo_album_generate_album_ui_image(
			out_image, album_list_text, output_cache_ui_file_name, page, loader, ref_w, ref_h);
	}
	else {
		int w, h, c;
		stbi_uc *px = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &w, &h, &c, 3);
		free(buf);

		if (px == NULL) goto GENERATE;
		else {
			*ref_w = (size_t)w;
			*ref_h = (size_t)h;
			return cpymo_backend_image_load(out_image, px, w, h, cpymo_backend_image_format_rgb);
		}
	}

}