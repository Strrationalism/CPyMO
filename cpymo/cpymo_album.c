#include <stb_image_resize.h>
#include <stb_image_write.h>
#include <stb_image.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "cpymo_error.h"
#include "cpymo_parser.h"
#include <cpymo_backend_image.h>
#include "cpymo_assetloader.h"
#include "cpymo_engine.h"
#include <assert.h>
#include <ctype.h>
#include "cpymo_album.h"

#define CPYMO_ALBUM_MAX_CGS_SINGLE_PAGE 25

static error_t cpymo_album_generate_album_ui_image_pixels(
	void **out_image, 
	cpymo_parser_stream_span album_list_text, 
	cpymo_parser_stream_span output_cache_ui_file_name,
	size_t page, 
	cpymo_assetloader* loader,
	size_t *ref_w, size_t *ref_h)
{
	stbi_uc *pixels = NULL;

	{
		char *path = (char *)malloc(strlen(loader->gamedir) + 15 + output_cache_ui_file_name.len);
		if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;
		strcpy(path, loader->gamedir);
		strcat(path, "/system/");
		strncat(path, output_cache_ui_file_name.begin, output_cache_ui_file_name.len);
		strcat(path, ".png");

		int w2, h2;
		pixels = stbi_load(path, &w2, &h2, NULL, 3);
		free(path);

		if (pixels != NULL) {
			*ref_w = (size_t)w2;
			*ref_h = (size_t)h2;
		}
		else {
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
		void *thumb_pixels = NULL;
		for (int i = 0; i < thumb_count; ++i) {
			// Try load ONE thumb.
			cpymo_parser_stream_span bgname_span = 
				cpymo_parser_curline_pop_commacell(&album_list_parser);
			cpymo_parser_stream_span_trim(&bgname_span);
		
			error_t err = cpymo_assetloader_load_bg_pixels(
				&thumb_pixels, &cg_w, &cg_h, bgname_span, loader);

			if (err != CPYMO_ERR_SUCC) break;
		}

		if (thumb_pixels == NULL) continue;

		stbir_resize_uint8(
			(stbi_uc *)thumb_pixels, cg_w, cg_h, cg_w * 3, 
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
			snprintf(page_str, sizeof(page_str), "%d", (int)page);
			strcat(path, page_str);
			strcat(path, ".png");
			stbi_write_png(path, (int)*ref_w, (int)*ref_h, 3, pixels, (int)*ref_w * 3);
			free(path);
		}
	}

	*out_image = pixels;

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_album_generate_album_ui_image(
	cpymo_backend_image *out_image, 
	cpymo_parser_stream_span album_list_text, 
	cpymo_parser_stream_span output_cache_ui_file_name,
	size_t page, 
	cpymo_assetloader* loader,
	size_t *ref_w, size_t *ref_h)
{
	void *pixels = NULL;
	error_t err = cpymo_album_generate_album_ui_image_pixels(
		&pixels,
		album_list_text,
		output_cache_ui_file_name,
		page,
		loader,
		ref_w, 
		ref_h);

	CPYMO_THROW(err);

	err = cpymo_backend_image_load(
		out_image, pixels, (int)*ref_w, (int)*ref_h, 
		cpymo_backend_image_format_rgb);

	if (err != CPYMO_ERR_SUCC) {
		free(pixels);
		return err;
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_album_load_ui_image(
	cpymo_backend_image *out_image,
	cpymo_parser_stream_span album_list_text,
	cpymo_parser_stream_span ui_file_name,
	size_t page,
	cpymo_assetloader *loader,
	size_t *ref_w, size_t *ref_h) 
{
	char *assetname = (char *)malloc(ui_file_name.len + 16);
	if (assetname == NULL) return CPYMO_ERR_OUT_OF_MEM;

	cpymo_parser_stream_span_copy(assetname, ui_file_name.len + 16, ui_file_name);
	char page_str[4];
	snprintf(page_str, sizeof(page_str), "%d", (int)page);
	strcat(assetname, "_");
	strcat(assetname, page_str);

	int w, h;
	error_t err = cpymo_assetloader_load_system_image(
		out_image, &w, &h, cpymo_parser_stream_span_pure(assetname), loader, false);
	free(assetname);

	if (err != CPYMO_ERR_SUCC) {
		return cpymo_album_generate_album_ui_image(
			out_image, album_list_text, ui_file_name, page, loader, ref_w, ref_h);
	}
	else {
		*ref_w = (size_t)w;
		*ref_h = (size_t)h;
		return CPYMO_ERR_SUCC;
	}
}

typedef struct {
	cpymo_backend_text title;
	float title_width;

	size_t cg_count;
	cpymo_parser cg_name_parser;
	bool force_unlock_all;
	bool preview_unlocked;
} cpymo_album_cg_info;

typedef struct {
	cpymo_backend_image current_ui, cv_thumb_cover;
	size_t current_ui_w, current_ui_h, cv_thumb_cover_w, cv_thumb_cover_h;

	size_t current_page, page_count, cg_count;
	cpymo_parser_stream_span album_list_name, album_ui_name;

	int current_cg_selection;

	cpymo_album_cg_info cg_infos[CPYMO_ALBUM_MAX_CGS_SINGLE_PAGE];

	char *album_list_text;
	size_t album_list_text_size;

	float mouse_wheel_sum;
	float mouse_x_sum;
	size_t ignore_mouse_button_release;

	cpymo_album_cg_info *showing_cg;
	cpymo_backend_image showing_cg_image;
	int showing_cg_image_w, showing_cg_image_h;
	cpymo_parser showing_cg_filename_parser;
	size_t showing_cg_next_cg_id;

	cpymo_key_pluse key_up, key_down, key_left, key_right;
} cpymo_album;

uint64_t cpymo_album_cg_name_hash(cpymo_parser_stream_span cg_filename)
{
	char cg_hash_str[64];
	strcpy(cg_hash_str, "CG:");
	for (size_t j = 0; j < cg_filename.len && j < 61; ++j)
		cg_hash_str[j + 3] = (char)toupper(cg_filename.begin[j]);

	cpymo_parser_stream_span span;
	span.begin = cg_hash_str;
	span.len = (size_t)cpymo_utils_clamp((int)cg_filename.len + 3, 0, 64);

	return cpymo_parser_stream_span_hash(span);
}

error_t cpymo_album_cg_unlock(cpymo_engine *e, cpymo_parser_stream_span cg_filename)
{
	if (!cpymo_parser_stream_span_starts_with_str_ignore_case(cg_filename, e->gameconfig.cgprefix))
		return CPYMO_ERR_SUCC;

	return cpymo_hash_flags_add(&e->flags, cpymo_album_cg_name_hash(cg_filename));
}

static error_t cpymo_album_load_page(cpymo_engine *e, cpymo_album *a)
{
	cpymo_engine_request_redraw(e);

	for (size_t i = 0; i < CPYMO_ALBUM_MAX_CGS_SINGLE_PAGE; ++i) {
		if (a->cg_infos[i].title != NULL)
			cpymo_backend_text_free(a->cg_infos[i].title);
		a->cg_infos[i].title = NULL;
	}

	if (a->current_ui != NULL) {
		cpymo_backend_image_free(a->current_ui);
		a->current_ui = NULL;
	}

	a->current_ui_w = e->gameconfig.imagesize_w;
	a->current_ui_h = e->gameconfig.imagesize_h;

	a->cg_count = 0;
	a->current_cg_selection = 0;

	cpymo_parser album_list;
	cpymo_parser_init(&album_list, a->album_list_text, a->album_list_text_size);
	do {
		if (a->cg_count >= CPYMO_ALBUM_MAX_CGS_SINGLE_PAGE) break;

		cpymo_parser_stream_span page_str = cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_parser_stream_span_trim(&page_str);

		if (page_str.len <= 0) continue;
		if (cpymo_parser_stream_span_atoi(page_str) != (int)a->current_page + 1) continue;

		cpymo_album_cg_info *cg_info = &a->cg_infos[a->cg_count++];

		cpymo_parser_stream_span thumb_count_str =
			cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_parser_stream_span_trim(&thumb_count_str);

		if (thumb_count_str.len <= 0) continue;
		const int thumb_count = cpymo_parser_stream_span_atoi(thumb_count_str);

		if (thumb_count < 1) continue;

		cg_info->cg_count = (size_t)thumb_count;

		cpymo_parser_stream_span cg_title =
			cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_parser_stream_span_trim(&cg_title);

		if (cg_title.len > 0) {
			error_t err = cpymo_backend_text_create(
				&cg_info->title,
				&cg_info->title_width,
				cg_title,
				cpymo_gameconfig_font_size(&e->gameconfig));

			if (err != CPYMO_ERR_SUCC) {
				cg_info->title = NULL;
				return err;
			}
		}

		cg_info->cg_name_parser = album_list;
		cg_info->preview_unlocked = false;

		for (int i = 0; i < thumb_count; ++i) {
			cpymo_parser_stream_span cg_filename = 
				cpymo_parser_curline_pop_commacell(&album_list);
			cpymo_parser_stream_span_trim(&cg_filename);
			uint64_t cg_hash = cpymo_album_cg_name_hash(cg_filename);

			if (cg_info->preview_unlocked == false) {
				if (cpymo_hash_flags_check(&e->flags, cg_hash))
					cg_info->preview_unlocked = true;
			}
		}

		cpymo_parser_stream_span force_unlock_all_flag = cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_parser_stream_span_trim(&force_unlock_all_flag);
		cg_info->force_unlock_all = cpymo_parser_stream_span_equals_str(force_unlock_all_flag, "1");

		if (cg_info->force_unlock_all)
			cg_info->preview_unlocked = true;

	} while (cpymo_parser_next_line(&album_list));

	assert(a->cg_count <= CPYMO_ALBUM_MAX_CGS_SINGLE_PAGE);

	cpymo_parser_stream_span span;
	span.begin = a->album_list_text;
	span.len = a->album_list_text_size;

	error_t err = cpymo_album_load_ui_image(
		&a->current_ui,
		span,
		a->album_ui_name,
		a->current_page,
		&e->assetloader,
		&a->current_ui_w,
		&a->current_ui_h);

	return err;
}

static error_t cpymo_album_next_page(cpymo_engine *e, cpymo_album *a)
{
	size_t prev_page = a->current_page;
	a->current_page = (a->current_page + 1) % a->page_count;
	if (prev_page != a->current_page) 
		return cpymo_album_load_page(e, a);
	else {
		a->current_cg_selection = 0;
		cpymo_engine_request_redraw(e);
		return CPYMO_ERR_SUCC;
	}
}

static error_t cpymo_album_prev_page(cpymo_engine *e, cpymo_album *a)
{
	size_t prev_page = a->current_page;
	if (a->current_page <= 0) a->current_page = a->page_count - 1;
	else a->current_page--;

	if (prev_page != a->current_page) return cpymo_album_load_page(e, a);
	else {
		a->current_cg_selection = 0;
		cpymo_engine_request_redraw(e);
		return CPYMO_ERR_SUCC;
	}
}

#ifndef __3DS__
static bool cpymo_album_check_mouse_in_box(cpymo_engine *e, size_t i) {
	size_t row = i / 5;
	size_t col = i % 5;
	float
		x = (float)e->gameconfig.imagesize_w * (0.03f + 0.19f * col),
		y = (float)e->gameconfig.imagesize_h * (0.02f + 0.19f * row),
		w = (float)e->gameconfig.imagesize_w * 0.17f,
		h = (float)e->gameconfig.imagesize_h * 0.17f,
		mx = e->input.mouse_x,
		my = e->input.mouse_y;

	float x2 = x + w, y2 = y + h;
	return mx >= x && mx <= x2 && my >= y && my <= y2;
}
#endif

static void cpymo_album_exit_showing_cg(cpymo_album *a) {
	if (a->showing_cg_image != NULL) {
		cpymo_backend_image_free(a->showing_cg_image);
		a->showing_cg_image = NULL;
	}

	a->showing_cg = NULL;
}

static void cpymo_album_showing_cg_next(cpymo_engine *e, cpymo_album *a) {
	if (a->showing_cg_image != NULL) {
		cpymo_backend_image_free(a->showing_cg_image);
		a->showing_cg_image = NULL;
	}

	cpymo_engine_request_redraw(e);

	size_t cg_id = a->showing_cg_next_cg_id++;
	if (cg_id >= a->cg_count) {
		a->ignore_mouse_button_release++;
		cpymo_album_exit_showing_cg(a);
	}
	else {
		cpymo_parser_stream_span filename =
			cpymo_parser_curline_pop_commacell(&a->showing_cg_filename_parser);
		cpymo_parser_stream_span_trim(&filename);
		uint64_t cghash = cpymo_album_cg_name_hash(filename);
		if (cpymo_hash_flags_check(&e->flags, cghash)) {
			error_t err = cpymo_assetloader_load_bg_image(
				&a->showing_cg_image,
				&a->showing_cg_image_w,
				&a->showing_cg_image_h,
				filename,
				&e->assetloader);

			if (err != CPYMO_ERR_SUCC) {
				a->showing_cg_image = NULL;
				cpymo_album_showing_cg_next(e, a);
			}
		}
		else
			cpymo_album_showing_cg_next(e, a);
	}
}

static error_t cpymo_album_select_ok(cpymo_engine *e, cpymo_album *a)
{
	a->showing_cg = &a->cg_infos[a->current_cg_selection];
	a->showing_cg_filename_parser = a->showing_cg->cg_name_parser;
	a->showing_cg_next_cg_id = 0;
	cpymo_album_showing_cg_next(e, a);
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_album_update(cpymo_engine *e, void *a, float dt)
{
	cpymo_album *album = (cpymo_album *)a;

	cpymo_key_pluse_update(&album->key_left, dt, e->input.left);
	cpymo_key_pluse_update(&album->key_up, dt, e->input.up);
	cpymo_key_pluse_update(&album->key_right, dt, e->input.right);
	cpymo_key_pluse_update(&album->key_down, dt, e->input.down);

	if (album->showing_cg) {
		if (CPYMO_INPUT_JUST_PRESSED(e, ok) || CPYMO_INPUT_JUST_PRESSED(e, mouse_button)) {
			cpymo_album_showing_cg_next(e, album);
			return CPYMO_ERR_SUCC;
		}

		if (CPYMO_INPUT_JUST_RELEASED(e, cancel)) {
			cpymo_engine_request_redraw(e);
			cpymo_album_exit_showing_cg(album);
			return CPYMO_ERR_SUCC;
		}

		return CPYMO_ERR_SUCC;
	}

	if (CPYMO_INPUT_JUST_RELEASED(e, cancel)) {
		cpymo_ui_exit(e);
		return CPYMO_ERR_SUCC;
	}

#ifndef __3DS__
	if (CPYMO_INPUT_JUST_RELEASED(e, mouse_button)) {
		float percent = album->mouse_x_sum / (float)e->gameconfig.imagesize_w;
		if (percent >= 0.25f) {
			album->mouse_x_sum = 0;
			return cpymo_album_prev_page(e, album);
		}
		else if (percent <= -0.25f) {
			album->mouse_x_sum = 0;
			return cpymo_album_next_page(e, album);
		}
		else if (album->ignore_mouse_button_release)
			album->ignore_mouse_button_release--;
		else {
			for (size_t i = 0; i < album->cg_count; ++i) {
				if (cpymo_album_check_mouse_in_box(e, i)) {
					album->current_cg_selection = (int)i;
					return cpymo_album_select_ok(e, album);
				}
			}
		}
	}

	if (e->input.mouse_button && e->prev_input.mouse_button) {
		if (e->input.mouse_position_useable) {
			album->mouse_x_sum += e->input.mouse_x - e->prev_input.mouse_x;
		}
	}
	else
		album->mouse_x_sum = 0;
#endif

	if (CPYMO_INPUT_JUST_PRESSED(e, ok)) return cpymo_album_select_ok(e, album);
	if (cpymo_key_pluse_output(&album->key_left)) return cpymo_album_prev_page(e, album);
	if (cpymo_key_pluse_output(&album->key_right)) return cpymo_album_next_page(e, album);

	if (cpymo_key_pluse_output(&album->key_down)) {
		album->current_cg_selection++;
		cpymo_engine_request_redraw(e);

		if (album->current_cg_selection >= (int)album->cg_count)
			return cpymo_album_next_page(e, album);
		return CPYMO_ERR_SUCC;
	}

	if (cpymo_key_pluse_output(&album->key_up)) {
		cpymo_engine_request_redraw(e);
		album->current_cg_selection--;
		if (album->current_cg_selection < 0) {
			error_t err = cpymo_album_prev_page(e, album);
			album->current_cg_selection = (int)album->cg_count - 1;
			return err;
		}
		return CPYMO_ERR_SUCC;
	}

	album->mouse_wheel_sum += e->input.mouse_wheel_delta;

	if (album->mouse_wheel_sum >= 1.0f) {
		album->mouse_wheel_sum -= 1.0f;
		return cpymo_album_prev_page(e, album);
	}

	if (album->mouse_wheel_sum <= -1.0f) {
		album->mouse_wheel_sum += 1.0f;
		return cpymo_album_next_page(e, album);
	}

#ifndef __3DS__
	if (e->prev_input.mouse_position_useable && e->input.mouse_position_useable) {
		if (e->prev_input.mouse_x != e->input.mouse_x || e->prev_input.mouse_y != e->input.mouse_y) {
			for (size_t i = 0; i < album->cg_count; ++i) {
				if (cpymo_album_check_mouse_in_box(e, i)) {
					cpymo_engine_request_redraw(e);
					album->current_cg_selection = (int)i;
					break;
				}
			}
		}
	}
#endif

	return CPYMO_ERR_SUCC;
}

static void cpymo_album_draw(const cpymo_engine *e, const void *_a)
{
	const cpymo_album *a = (const cpymo_album *)_a;

	if (a->showing_cg) {
		if (a->showing_cg_image) {
			cpymo_backend_image_draw(
				0, 0, (float)e->gameconfig.imagesize_w, (float)e->gameconfig.imagesize_h,
				a->showing_cg_image, 0, 0, a->showing_cg_image_w, a->showing_cg_image_h,
				1.0f, cpymo_backend_image_draw_type_bg);
			return;
		}
	}
	
	if (a->current_ui) {
		cpymo_backend_image_draw(
			0, 0, e->gameconfig.imagesize_w, e->gameconfig.imagesize_h,
			a->current_ui, 0, 0, (int)a->current_ui_w, (int)a->current_ui_h,
			1.0f, cpymo_backend_image_draw_type_bg);
	}

	for (size_t i = 0; i < a->cg_count; ++i) {
		size_t row = i / 5;
		size_t col = i % 5;
		float xywh[] = {
			(float)e->gameconfig.imagesize_w * (0.03f + 0.19f * (float)col),
			(float)e->gameconfig.imagesize_h * (0.02f + 0.19f * (float)row),
			(float)e->gameconfig.imagesize_w * 0.17f,
			(float)e->gameconfig.imagesize_h * 0.17f
		};

		const cpymo_album_cg_info *info = &a->cg_infos[i];
		if (info->preview_unlocked == false) {
			if (a->cv_thumb_cover == NULL) {
				cpymo_backend_image_fill_rects(
					xywh, 1, cpymo_color_black, 0.5f, cpymo_backend_image_draw_type_bg);
			}
			else {
				cpymo_backend_image_draw(
					xywh[0], xywh[1], xywh[2], xywh[3],
					a->cv_thumb_cover, 0, 0, (int)a->cv_thumb_cover_w, (int)a->cv_thumb_cover_h,
					1.0f, cpymo_backend_image_draw_type_bg);
			}
		}

		if((int)i == a->current_cg_selection)
			cpymo_backend_image_fill_rects(xywh, 1, cpymo_color_white, 0.5f, cpymo_backend_image_draw_type_bg);
	}
}

static void cpymo_album_deleter(cpymo_engine *e, void *a)
{
	cpymo_album *album = (cpymo_album *)a;
	if (album->album_list_text) free(album->album_list_text);
	if (album->current_ui) cpymo_backend_image_free(album->current_ui);
	if (album->cv_thumb_cover) cpymo_backend_image_free(album->cv_thumb_cover);

	for (size_t i = 0; i < CPYMO_ALBUM_MAX_CGS_SINGLE_PAGE; ++i)
		if (album->cg_infos[i].title != NULL)
			cpymo_backend_text_free(album->cg_infos[i].title);
}

error_t cpymo_album_enter(
	cpymo_engine *e, 
	cpymo_parser_stream_span album_list_name,
	cpymo_parser_stream_span album_ui_name,
	size_t page)
{
	cpymo_album *album = NULL;
	error_t err = cpymo_ui_enter(
		(void **)&album, 
		e, 
		sizeof(cpymo_album),
		&cpymo_album_update,
		&cpymo_album_draw, 
		&cpymo_album_deleter);
	CPYMO_THROW(err);

	album->album_list_name = album_list_name;
	album->album_ui_name = album_ui_name;
	album->current_ui = NULL;
	album->current_page = page;
	album->page_count = 0;
	album->mouse_wheel_sum = 0;
	album->cv_thumb_cover = NULL;
	album->mouse_x_sum = 0;
	album->ignore_mouse_button_release = 0;
	album->showing_cg = NULL;
	album->showing_cg_image = NULL;

	cpymo_key_pluse_init(&album->key_left, e->input.left);
	cpymo_key_pluse_init(&album->key_right, e->input.right);
	cpymo_key_pluse_init(&album->key_up, e->input.up);
	cpymo_key_pluse_init(&album->key_down, e->input.down);

	int w, h;
	err = cpymo_assetloader_load_system_image(
		&album->cv_thumb_cover,
		&w,
		&h,
		cpymo_parser_stream_span_pure("cvThumb"),
		&e->assetloader,
		false);

	if (err == CPYMO_ERR_SUCC) {
		album->cv_thumb_cover_w = (size_t)w;
		album->cv_thumb_cover_h = (size_t)h;
	}
	
	for (size_t i = 0; i < CPYMO_ALBUM_MAX_CGS_SINGLE_PAGE; ++i) {
		album->cg_infos[i].title = NULL;
	}

	char script_name[128];
	cpymo_parser_stream_span_copy(script_name, sizeof(script_name), album_list_name);

	album->album_list_text = NULL;
	err = cpymo_assetloader_load_script(
		&album->album_list_text,
		&album->album_list_text_size, 
		script_name, &e->assetloader);

	cpymo_parser parser;
	cpymo_parser_init(&parser, album->album_list_text, album->album_list_text_size);
	do {
		cpymo_parser_stream_span span = cpymo_parser_curline_pop_commacell(&parser);
		cpymo_parser_stream_span_trim(&span);
		if (span.len > 0) {
			size_t page_id = (size_t)cpymo_parser_stream_span_atoi(span);
			if (page_id > album->page_count) album->page_count = page_id;
		}
	} while (cpymo_parser_next_line(&parser));

	CPYMO_THROW(err);

	return cpymo_album_load_page(e, album);
}

