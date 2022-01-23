#include <stb_image_resize.h>
#include <stb_image_write.h>
#include <stb_image.h>
#include <memory.h>
#include "cpymo_error.h"
#include "cpymo_parser.h"
#include <cpymo_backend_image.h>
#include "cpymo_assetloader.h"

error_t cpymo_album_generate_album_ui_image(
	cpymo_backend_image *out_image, 
	cpymo_parser_stream_span album_list, 
	size_t page, 
	cpymo_assetloader* loader,
	size_t w, size_t h)
{
	stbi_uc *pixels = NULL;

	{	// Load album bg
		char *image_buf = NULL;
		size_t image_buf_size = 0;
		error_t err = cpymo_assetloader_load_system(&image_buf, &image_buf_size, "albumbg", "png", loader);
		if (err == CPYMO_ERR_SUCC) {
			int w2, h2, channels;
			stbi_uc *pixels = stbi_load_from_memory((stbi_uc *)image_buf, (int)image_buf_size, &w2, &h2, &channels, 3);
			free(image_buf);

			if (pixels != NULL) {
				w = (size_t)w2;
				h = (size_t)h2;
			}
		}

		if (pixels == NULL) {
			pixels = (stbi_uc *)malloc(w * h * 3);
			if (pixels == NULL) return CPYMO_ERR_OUT_OF_MEM;
			memset(pixels, 0, w * h * 3);
		}
	}

	cpymo_parser album_list_parser;
	cpymo_parser_init(&album_list_parser, album_list.begin, album_list.len);

	size_t next_id = 0;

	const size_t thumb_width = (size_t)(0.17 * (double)w);
	const size_t thumb_height = (size_t)(0.17 * (double)h);

	do {
		cpymo_parser_stream_span page_str = cpymo_parser_curline_pop_commacell(&album_list_parser);
		cpymo_parser_stream_span_trim(&page_str);

		if (page_str.len <= 0) continue;
		if (cpymo_parser_stream_span_atoi(page_str) != (int)page) continue;

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
		const size_t thumb_left_top_x = (size_t)((0.03 + 0.19 * col) * (double)w);
		const size_t thumb_left_top_y = (size_t)((0.02 + 0.19 * col) * (double)h);
		const size_t thumb_right_bottom_x = thumb_left_top_x + thumb_width;
		const size_t thumb_right_bottom_y = thumb_left_top_y + thumb_height;

		if (thumb_right_bottom_x >= w || thumb_right_bottom_y >= h) continue;

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
			thumb_pixels = stbi_load_from_memory(buf, (int)buf_size, &cg_w, &cg_h, &cg_channels, 3);
			free(buf);

			if (thumb_pixels != NULL) break;
		}

		if (thumb_pixels == NULL) continue;

		// resize and write to album here

	} while (cpymo_parser_next_line(&album_list_parser));

	// *out_image = ...
	// write to game data package.

	free(pixels);
	return CPYMO_ERR_SUCC;
}