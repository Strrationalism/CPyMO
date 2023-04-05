#include "cpymo_prelude.h"
#include "../stb/stb_image_resize.h"
#include "../stb/stb_image_write.h"
#include "../stb/stb_image.h"
#include <memory.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "cpymo_error.h"
#include "cpymo_parser.h"
#include "cpymo_assetloader.h"
#include <assert.h>
#include <ctype.h>

#define ALBUM_MAX_CGS_SINGLE_PAGE 25
const static float album_scroll_time = 3.0f;

#ifdef CPYMO_TOOL
bool cpymo_backend_image_album_ui_writable(void);
#endif

#ifdef __3DS__
extern bool fill_screen_enabled;
#endif

#ifndef DISABLE_STB_IMAGE

static void cpymo_album_generate_album_ui_image_pixels_cut(
	void **pixels, int *w, int *h, float ratio)
{
	float cur_ratio = (float)*w / (float)*h;
	int new_w = *w;
	int new_h = *h;

	assert(new_w >= 0 && new_h >= 0);

	if (cur_ratio > ratio) new_w = (int)(*h * ratio);
	else if (cur_ratio < ratio) new_h = (int)(*w / ratio);
	if (new_w == *w && new_h == *h) return;

	uint8_t *pixels_cut = (uint8_t *)malloc(new_w * new_h * 3);
	if (pixels_cut == NULL) return;

	uint8_t *pixels_src = (uint8_t *)*pixels;

	for (size_t y = 0; y < (size_t)new_h; ++y) {
		size_t copy_count = 3 * new_w;
		uint8_t *copy_src = pixels_src + 3 * *w * y;
		uint8_t *copy_dst = pixels_cut + 3 * new_w * y;
		memcpy(copy_dst, copy_src, copy_count);
	}

	free(*pixels);
	*pixels = pixels_cut;
	*w = new_w;
	*h = new_h;
}

error_t cpymo_album_generate_album_ui_image_pixels(
	void **out_image, 
	cpymo_str album_list_text, 
	cpymo_str output_cache_ui_file_name,
	size_t page, 
	cpymo_assetloader* loader,
	size_t *ref_w, size_t *ref_h)
{
	stbi_uc *pixels = NULL;

	{
		char *path = NULL;

		error_t err = cpymo_assetloader_get_fs_path(
			&path,
			output_cache_ui_file_name,
			"system",
			"png",
			loader);

		CPYMO_THROW(err);

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
		cpymo_str page_str = cpymo_parser_curline_pop_commacell(&album_list_parser);
		cpymo_str_trim(&page_str);

		if (page_str.len <= 0) continue;
		if (cpymo_str_atoi(page_str) != (int)page + 1) continue;

		const size_t id = next_id++;

		cpymo_str thumb_count_str = 
			cpymo_parser_curline_pop_commacell(&album_list_parser);
		cpymo_str_trim(&thumb_count_str);
		if (thumb_count_str.len <= 0) continue;
		const int thumb_count = cpymo_str_atoi(thumb_count_str);

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
			cpymo_str bgname_span = 
				cpymo_parser_curline_pop_commacell(&album_list_parser);
			cpymo_str_trim(&bgname_span);
		
			error_t err = cpymo_assetloader_load_bg_pixels(
				&thumb_pixels, &cg_w, &cg_h, bgname_span, loader);

			if (err == CPYMO_ERR_SUCC) break;
		}

		if (thumb_pixels == NULL) continue;

		cpymo_album_generate_album_ui_image_pixels_cut(
			&thumb_pixels, &cg_w, &cg_h, 
			(float)thumb_width / (float)thumb_height);

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

#else
static error_t cpymo_album_generate_album_ui_image_pixels(
	void **out_image, 
	cpymo_str album_list_text, 
	cpymo_str output_cache_ui_file_name,
	size_t page, 
	cpymo_assetloader* loader,
	size_t *ref_w, size_t *ref_h)
{
	return CPYMO_ERR_UNSUPPORTED;
}
#endif

#ifndef CPYMO_TOOL
#include "../cpymo-backends/include/cpymo_backend_image.h"
#include "cpymo_album.h"
#include "cpymo_key_hold.h"
#include "cpymo_engine.h"

static error_t cpymo_album_generate_album_ui_image(
	cpymo_backend_image *out_image, 
	cpymo_str album_list_text, 
	cpymo_str output_cache_ui_file_name,
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
	cpymo_str album_list_text,
	cpymo_str ui_file_name,
	size_t page,
	cpymo_assetloader *loader,
	size_t *ref_w, size_t *ref_h) 
{
	char *assetname = (char *)malloc(ui_file_name.len + 16);
	if (assetname == NULL) return CPYMO_ERR_OUT_OF_MEM;

	cpymo_str_copy(assetname, ui_file_name.len + 16, ui_file_name);
	char page_str[4];
	snprintf(page_str, sizeof(page_str), "%d", (int)page);
	strcat(assetname, "_");
	strcat(assetname, page_str);

	int w, h;
	error_t err = cpymo_assetloader_load_system_image(
		out_image, &w, &h, cpymo_str_pure(assetname), loader, false);
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
	cpymo_str album_list_name, album_ui_name;

	int current_cg_selection;

	cpymo_album_cg_info cg_infos[ALBUM_MAX_CGS_SINGLE_PAGE];

	char *album_list_text;
	size_t album_list_text_size;

	float mouse_wheel_sum;
	float mouse_x_sum;

	cpymo_album_cg_info *showing_cg;
	cpymo_backend_image showing_cg_image;
	int showing_cg_image_w, showing_cg_image_h;
	cpymo_parser showing_cg_filename_parser;
	size_t showing_cg_next_cg_id;

	cpymo_key_pluse key_up, key_down, key_left, key_right;
	cpymo_key_hold key_mouse_button;

	float showing_cg_image_draw_src_end_x,
		  showing_cg_image_draw_src_end_y;

#ifndef LOW_FRAME_RATE
	cpymo_tween showing_cg_draw_src_progress;
#else
	bool showing_cg_show_end;
#endif

	int showing_cg_image_draw_src_w,
		showing_cg_image_draw_src_h;

	char *current_bg_name;
	float current_bg_x, current_bg_y;
} cpymo_album;

uint64_t cpymo_album_cg_name_hash(cpymo_str cg_filename)
{
	uint64_t hash;
	cpymo_str_hash_init(&hash);
	cpymo_str_hash_append_cstr(&hash, "CG:");

	for (size_t j = 0; j < cg_filename.len && j < 61; ++j)
		cpymo_str_hash_step(
			&hash, (char)toupper(cg_filename.begin[j]));

	return hash;
}

error_t cpymo_album_cg_unlock(cpymo_engine *e, cpymo_str cg_filename)
{
	if (!cpymo_str_starts_with_str_ignore_case(cg_filename, e->gameconfig.cgprefix))
		return CPYMO_ERR_SUCC;

	return cpymo_hash_flags_add(&e->flags, cpymo_album_cg_name_hash(cg_filename));
}

static void cpymo_album_unload_page(cpymo_engine *e, cpymo_album *a)
{
	for (size_t i = 0; i < ALBUM_MAX_CGS_SINGLE_PAGE; ++i) {
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

	if (a->cv_thumb_cover) {
		cpymo_backend_image_free(a->cv_thumb_cover);
		a->cv_thumb_cover = NULL;
		a->cv_thumb_cover_w = 0;
		a->cv_thumb_cover_h = 0;
	}
}

static error_t cpymo_album_load_page(cpymo_engine *e, cpymo_album *a)
{
	cpymo_engine_request_redraw(e);
	cpymo_album_unload_page(e, a);

	a->cg_count = 0;

	cpymo_parser album_list;
	cpymo_parser_init(&album_list, a->album_list_text, a->album_list_text_size);
	do {
		if (a->cg_count >= ALBUM_MAX_CGS_SINGLE_PAGE) break;

		cpymo_str page_str = cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_str_trim(&page_str);

		if (page_str.len <= 0) continue;
		if (cpymo_str_atoi(page_str) != (int)a->current_page + 1) continue;

		cpymo_album_cg_info *cg_info = &a->cg_infos[a->cg_count++];

		cpymo_str thumb_count_str =
			cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_str_trim(&thumb_count_str);

		if (thumb_count_str.len <= 0) continue;
		const int thumb_count = cpymo_str_atoi(thumb_count_str);

		if (thumb_count < 1) continue;

		cg_info->cg_count = (size_t)thumb_count;

		cpymo_str cg_title =
			cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_str_trim(&cg_title);

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
			cpymo_str cg_filename = 
				cpymo_parser_curline_pop_commacell(&album_list);
			cpymo_str_trim(&cg_filename);
			uint64_t cg_hash = cpymo_album_cg_name_hash(cg_filename);

			if (cg_info->preview_unlocked == false) {
				if (cpymo_hash_flags_check(&e->flags, cg_hash))
					cg_info->preview_unlocked = true;
			}
		}

		cpymo_str force_unlock_all_flag = cpymo_parser_curline_pop_commacell(&album_list);
		cpymo_str_trim(&force_unlock_all_flag);
		cg_info->force_unlock_all = cpymo_str_equals_str(force_unlock_all_flag, "1");

		if (cg_info->force_unlock_all)
			cg_info->preview_unlocked = true;

	} while (cpymo_parser_next_line(&album_list));

	assert(a->cg_count <= ALBUM_MAX_CGS_SINGLE_PAGE);

	cpymo_str span;
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

	for (size_t i = 0; i < a->cg_count; ++i) {
		if (!a->cg_infos[i].preview_unlocked) {
			int w, h;
			assert(a->cv_thumb_cover == NULL);
			error_t err = cpymo_assetloader_load_system_image(
				&a->cv_thumb_cover,
				&w,
				&h,
				cpymo_str_pure("cvThumb"),
				&e->assetloader,
				false);

			if (err == CPYMO_ERR_SUCC) {
				a->cv_thumb_cover_w = (size_t)w;
				a->cv_thumb_cover_h = (size_t)h;
			}
			else {
				a->cv_thumb_cover = NULL;
				a->cv_thumb_cover_w = 0;
				a->cv_thumb_cover_h = 0;
			}

			break;
		}
	}

	return err;
}

static error_t cpymo_album_next_page(cpymo_engine *e, cpymo_album *a)
{
	a->current_cg_selection = 0;
	size_t prev_page = a->current_page;
	a->current_page = (a->current_page + 1) % a->page_count;
	if (prev_page != a->current_page) 
		return cpymo_album_load_page(e, a);
	else {
		cpymo_engine_request_redraw(e);
		return CPYMO_ERR_SUCC;
	}
}

static error_t cpymo_album_prev_page(cpymo_engine *e, cpymo_album *a)
{
	a->current_cg_selection = 0;
	size_t prev_page = a->current_page;
	if (a->current_page <= 0) a->current_page = a->page_count - 1;
	else a->current_page--;

	if (prev_page != a->current_page) 
		return cpymo_album_load_page(e, a);
	else {
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

static void cpymo_album_exit_showing_cg(cpymo_engine *e, cpymo_album *a) {
	if (a->showing_cg_image != NULL) {
		cpymo_backend_image_free(a->showing_cg_image);
		a->showing_cg_image = NULL;
	}

	a->showing_cg = NULL;
	cpymo_album_load_page(e, a);
	cpymo_input_ignore_next_mouse_button_event(e);
}

static void cpymo_album_showing_cg_next(
	cpymo_engine *e, cpymo_album *a) {
	cpymo_input_ignore_next_mouse_button_event(e);
	if (a->showing_cg_image != NULL) {
		cpymo_backend_image_free(a->showing_cg_image);
		a->showing_cg_image = NULL;
	}

	cpymo_engine_request_redraw(e);

	size_t cg_id = a->showing_cg_next_cg_id++;
	if (cg_id >= a->cg_count) {
		cpymo_album_exit_showing_cg(e, a);
	}
	else {
		cpymo_str filename =
			cpymo_parser_curline_pop_commacell(&a->showing_cg_filename_parser);
		cpymo_str_trim(&filename);
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
				printf("[Error] %s\n", cpymo_error_message(err));
				cpymo_album_showing_cg_next(e, a);
				return;
			}

			a->showing_cg_image_draw_src_w = a->showing_cg_image_w;
			a->showing_cg_image_draw_src_h = a->showing_cg_image_h;

			float screen_ratio = 
				(float)e->gameconfig.imagesize_w /
				(float)e->gameconfig.imagesize_h;

			float cur_ratio =
				(float)a->showing_cg_image_draw_src_w /
				(float)a->showing_cg_image_draw_src_h;

			if (cur_ratio > screen_ratio) 
				a->showing_cg_image_draw_src_w = 
					(int)(screen_ratio * a->showing_cg_image_draw_src_h);
			else if (cur_ratio < screen_ratio) 
				a->showing_cg_image_draw_src_h = 
					(int)(a->showing_cg_image_draw_src_w / screen_ratio);

			a->showing_cg_image_draw_src_end_x =
				(float)(a->showing_cg_image_w - a->showing_cg_image_draw_src_w);
			a->showing_cg_image_draw_src_end_y =
				(float)(a->showing_cg_image_h - a->showing_cg_image_draw_src_h); 
				
			#ifndef LOW_FRAME_RATE
				if (a->showing_cg_image_draw_src_end_x < 5.0f &&
					a->showing_cg_image_draw_src_end_y < 5.0f)
					cpymo_tween_assign(&a->showing_cg_draw_src_progress, 1);
				else {
					cpymo_tween_assign(&a->showing_cg_draw_src_progress, 0);
					cpymo_tween_to(
						&a->showing_cg_draw_src_progress, 
						1.0f, album_scroll_time);
				}
			#else
				a->showing_cg_show_end = false;
			#endif

			cpymo_album_unload_page(e, a);
		}
		else
			cpymo_album_showing_cg_next(e, a);
	}
}

static error_t cpymo_album_select_ok(cpymo_engine *e, cpymo_album *a)
{
	cpymo_album_cg_info *cg_info = &a->cg_infos[a->current_cg_selection];
	if (!cg_info->preview_unlocked) return CPYMO_ERR_SUCC;
	a->showing_cg = cg_info;
	a->showing_cg_filename_parser = a->showing_cg->cg_name_parser;
	a->showing_cg_next_cg_id = 0;
	cpymo_album_showing_cg_next(e, a);
	cpymo_input_ignore_next_mouse_button_event(e);
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_album_update(cpymo_engine *e, void *a, float dt)
{
	cpymo_album *album = (cpymo_album *)a;

	enum cpymo_key_hold_result mouse_button_state = cpymo_key_hold_update(
		e, &album->key_mouse_button, dt, e->input.mouse_button);

	cpymo_key_pluse_update(&album->key_left, dt, e->input.left);
	cpymo_key_pluse_update(&album->key_up, dt, e->input.up);
	cpymo_key_pluse_update(&album->key_right, dt, e->input.right);
	cpymo_key_pluse_update(&album->key_down, dt, e->input.down);

	if (album->showing_cg) {
		#ifndef LOW_FRAME_RATE
			bool scroll_finished = 
				cpymo_tween_finished(&album->showing_cg_draw_src_progress);

			if (!scroll_finished) {
				cpymo_tween_update(&album->showing_cg_draw_src_progress, dt);
				cpymo_engine_request_redraw(e);
			}

		#endif

		if (CPYMO_INPUT_JUST_RELEASED(e, ok) || CPYMO_INPUT_JUST_RELEASED(e, mouse_button)) {
			#ifdef LOW_FRAME_RATE
				if (album->showing_cg_show_end) 
					cpymo_album_showing_cg_next(e, album);
				else {
					album->showing_cg_show_end = true;
					cpymo_engine_request_redraw(e);
				}
			#else
				if (scroll_finished) cpymo_album_showing_cg_next(e, album);
				else cpymo_tween_finish(&album->showing_cg_draw_src_progress);
			#endif
			return CPYMO_ERR_SUCC;
		}

		if (CPYMO_INPUT_JUST_RELEASED(e, cancel) || mouse_button_state == cpymo_key_hold_result_just_hold) {
			cpymo_engine_request_redraw(e);
			cpymo_album_exit_showing_cg(e, album);
			return CPYMO_ERR_SUCC;
		}

		return CPYMO_ERR_SUCC;
	}

	if (CPYMO_INPUT_JUST_RELEASED(e, cancel) || mouse_button_state == cpymo_key_hold_result_just_hold) {
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

	if (CPYMO_INPUT_JUST_RELEASED(e, ok)) return cpymo_album_select_ok(e, album);
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
					if (album->current_cg_selection != (int)i) {
						cpymo_engine_request_redraw(e);
						album->current_cg_selection = (int)i;
					}
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

	#ifdef __3DS__
	fill_screen_enabled = a->showing_cg;
	#endif

	if (a->showing_cg) {
		if (a->showing_cg_image) {
			float src_x = 0;
			float src_y = 0;
			
			#ifndef LOW_FRAME_RATE
				src_x = 
					cpymo_tween_value(&a->showing_cg_draw_src_progress) *
					a->showing_cg_image_draw_src_end_x;

				src_y = 
					cpymo_tween_value(&a->showing_cg_draw_src_progress) *
					a->showing_cg_image_draw_src_end_y;
			#else
				if (a->showing_cg_show_end) {
					src_x = a->showing_cg_image_draw_src_end_x;
					src_y = a->showing_cg_image_draw_src_end_y;
				}
			#endif

			cpymo_backend_image_draw(
				0, 0, (float)e->gameconfig.imagesize_w, (float)e->gameconfig.imagesize_h,
				a->showing_cg_image,
				(int)src_x,
				(int)src_y, 
				a->showing_cg_image_draw_src_w, 
				a->showing_cg_image_draw_src_h,
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

		if((int)i == a->current_cg_selection) {
			const float thick = 3;
			float highlight_rects[] = {
				xywh[0] - thick, xywh[1], thick, xywh[3],
				xywh[0] + xywh[2] , xywh[1], thick, xywh[3],
				xywh[0] - thick, xywh[1] - thick, xywh[2] + 2 * thick, thick,
				xywh[0] - thick, xywh[1] + xywh[3], xywh[2] + 2 * thick, thick
			};

			const cpymo_color col = { 0 , 255, 0 };

			cpymo_backend_image_fill_rects(
				highlight_rects, 
				CPYMO_ARR_COUNT(highlight_rects) / 4, 
				col, 
				1.0f, 
				cpymo_backend_image_draw_type_bg);

			#ifndef DISABLE_HIGHLIGHT_SQUARE
			cpymo_backend_image_fill_rects(
				xywh, 
				1, 
				cpymo_color_white, 
				0.5f, 
				cpymo_backend_image_draw_type_bg);
			#endif
		}
	}

	const cpymo_album_cg_info *info = &a->cg_infos[a->current_cg_selection];
	if (info->title) {
		float draw_y = e->gameconfig.imagesize_h / 4.0f;
		if (a->current_cg_selection < 15) draw_y *= 3;

		float xywh[] = {
			(e->gameconfig.imagesize_w - info->title_width) / 2 - 20,
			draw_y - cpymo_gameconfig_font_size(&e->gameconfig),
			info->title_width + 40,
			cpymo_gameconfig_font_size(&e->gameconfig) + 12.0f
		};

		cpymo_backend_image_fill_rects(
			xywh, 
			1, 
			cpymo_color_black, 
			0.35f, 
			cpymo_backend_image_draw_type_ui_element);

		cpymo_backend_text_draw(
			info->title,
			(e->gameconfig.imagesize_w - info->title_width) / 2,
			draw_y,
			cpymo_color_white,
			1.0f,
			cpymo_backend_image_draw_type_ui_element);
	}
}

static void cpymo_album_deleter(cpymo_engine *e, void *a)
{
	cpymo_album *album = (cpymo_album *)a;
	if (album->album_list_text) free(album->album_list_text);
	if (album->current_ui) cpymo_backend_image_free(album->current_ui);
	if (album->cv_thumb_cover) cpymo_backend_image_free(album->cv_thumb_cover);
	if (album->showing_cg) {
		if (album->showing_cg_image) 
			cpymo_backend_image_free(album->showing_cg_image);
	}

	for (size_t i = 0; i < ALBUM_MAX_CGS_SINGLE_PAGE; ++i)
		if (album->cg_infos[i].title != NULL)
			cpymo_backend_text_free(album->cg_infos[i].title);

	#ifdef __3DS__
	fill_screen_enabled = true;
	#endif

	
	if (album->current_bg_name) {
		cpymo_bg_command(
			e, 
			&e->bg, 
			cpymo_str_pure(album->current_bg_name),
			cpymo_str_pure("BG_NOFADE"),
			album->current_bg_x, album->current_bg_y, 0);
		free(album->current_bg_name);
	}
}

error_t cpymo_album_enter(
	cpymo_engine *e, 
	cpymo_str album_list_name,
	cpymo_str album_ui_name,
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
	album->mouse_x_sum = 0;
	album->showing_cg = NULL;
	album->showing_cg_image = NULL;
	album->current_bg_name = NULL;

	cpymo_key_pluse_init(&album->key_left, e->input.left);
	cpymo_key_pluse_init(&album->key_right, e->input.right);
	cpymo_key_pluse_init(&album->key_up, e->input.up);
	cpymo_key_pluse_init(&album->key_down, e->input.down);
	cpymo_key_hold_init(&album->key_mouse_button, e->input.mouse_button);

	album->cv_thumb_cover = NULL;
	album->cv_thumb_cover_w = 0;
	album->cv_thumb_cover_h = 0;
	
	for (size_t i = 0; i < ALBUM_MAX_CGS_SINGLE_PAGE; ++i) {
		album->cg_infos[i].title = NULL;
	}

	char script_name[128];
	cpymo_str_copy(script_name, sizeof(script_name), album_list_name);

	album->album_list_text = NULL;
	err = cpymo_assetloader_load_script(
		&album->album_list_text,
		&album->album_list_text_size, 
		script_name, &e->assetloader);

	if (err != CPYMO_ERR_SUCC) {
		cpymo_ui_exit(e);
		return err;
	}

	cpymo_parser parser;
	cpymo_parser_init(&parser, album->album_list_text, album->album_list_text_size);
	do {
		cpymo_str span = cpymo_parser_curline_pop_commacell(&parser);
		cpymo_str_trim(&span);
		if (span.len > 0) {
			size_t page_id = (size_t)cpymo_str_atoi(span);
			if (page_id > album->page_count) album->page_count = page_id;
		}
	} while (cpymo_parser_next_line(&parser));

	album->current_bg_name = e->bg.current_bg_name;
	e->bg.current_bg_name = NULL;
	album->current_bg_x = e->bg.current_bg_x;
	album->current_bg_y = e->bg.current_bg_y;
	cpymo_bg_reset(&e->bg);

	album->current_cg_selection = 0;
	album->current_page = 0;

	return cpymo_album_load_page(e, album);
}

#endif

