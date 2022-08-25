#include "cpymo_prelude.h"
#include "cpymo_music_box.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include "cpymo_parser.h"
#include <cpymo_backend_text.h>
#include <assert.h>
#include <stdlib.h>

#define ENCODE_NODE(INDEX) CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(INDEX)
#define DECODE_NODE(PTR) CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(PTR)

typedef struct {
	char *music_list;

	uintptr_t music_count;
	cpymo_str *music_filename;
	cpymo_backend_text *music_title;

#ifdef ENABLE_TEXT_EXTRACT
	char **music_title_text;
#endif

	float font_size;
} cpymo_music_box;

static void cpymo_music_box_deleter(cpymo_engine *e, void *ui_)
{
	cpymo_music_box *box = (cpymo_music_box *)ui_;

	for (uintptr_t i = 0; i < box->music_count; ++i)
		cpymo_backend_text_free(box->music_title[i]);

#ifdef ENABLE_TEXT_EXTRACT
	if (box->music_title_text) {
		for (uintptr_t i = 0; i < box->music_count; ++i)
			if (box->music_title_text[i]) free(box->music_title_text[i]);
		free(box->music_title_text);
	}
#endif

	free(box->music_list);
	free(box->music_filename);
}

static void *cpymo_music_box_get_next(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	cpymo_music_box *box = (cpymo_music_box *)ui_data;
	uintptr_t index = DECODE_NODE(cur) + 1;
	if (index >= box->music_count) return NULL;
	else return ENCODE_NODE(index);
}

static void *cpymo_music_box_get_prev(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t index = DECODE_NODE(cur);
	if (index == 0) return NULL;
	else return ENCODE_NODE(index - 1);
}

static void cpymo_musicbox_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{
	uintptr_t node_index = DECODE_NODE(node_to_draw);
	const cpymo_music_box *box = (cpymo_music_box *)cpymo_list_ui_data_const(e);
	
	cpymo_backend_text text = box->music_title[node_index];
	cpymo_backend_text_draw(
		text, 0, y + box->font_size, cpymo_color_white, 1.0f, 
		cpymo_backend_image_draw_type_ui_element);
}

static error_t cpymo_musicbox_ok(struct cpymo_engine *e, void *selected)
{
	const cpymo_music_box *box = (cpymo_music_box *)cpymo_list_ui_data_const(e);

	uintptr_t node_index = DECODE_NODE(selected);
	cpymo_audio_bgm_play(e, box->music_filename[node_index], true);
	return CPYMO_ERR_SUCC;
}

#ifdef ENABLE_TEXT_EXTRACT
static error_t cpymo_musicbox_visual_help_selection_change(cpymo_engine *e, void *selected)
{
	const cpymo_music_box *box = (cpymo_music_box *)cpymo_list_ui_data_const(e);
	
	if (box->music_title_text) {
		uintptr_t node_index = DECODE_NODE(selected);
		if (box->music_title_text[node_index])
			cpymo_backend_text_extract(box->music_title_text[node_index]);
	}

	return CPYMO_ERR_SUCC;
}
#endif

error_t cpymo_music_box_enter(cpymo_engine *e)
{
	cpymo_music_box *box = NULL;

	error_t err = cpymo_list_ui_enter(
		e, (void **)&box, sizeof(cpymo_music_box), 
		&cpymo_musicbox_draw_node,
		&cpymo_musicbox_ok,
		&cpymo_music_box_deleter, 
		(void *)0, 
		&cpymo_music_box_get_next, 
		&cpymo_music_box_get_prev, 
		false,
		5);
	CPYMO_THROW(err);

	cpymo_list_ui_enable_loop(e);

	box->music_list = NULL;
	size_t music_list_size = 0;
	err = cpymo_assetloader_load_script(
		&box->music_list, &music_list_size, 
		"music_list", &e->assetloader);
	CPYMO_THROW(err);

	cpymo_parser p;
	cpymo_parser_init(&p, box->music_list, music_list_size);

	box->music_count = 0;

	do {
		cpymo_str music_file =
			cpymo_parser_curline_pop_commacell(&p);
		cpymo_str_trim(&music_file);
		if (music_file.len) box->music_count++;
	} while (cpymo_parser_next_line(&p));

	if (box->music_count == 0) {
		free(box->music_list);
		box->music_list = NULL;
		return CPYMO_ERR_NO_MORE_CONTENT;
	}

	cpymo_parser_reset(&p);

	box->music_filename = 
		(cpymo_str *)malloc(
			(sizeof(cpymo_str) + sizeof(cpymo_backend_text)) * box->music_count);

	if (box->music_filename == NULL) {
		free(box->music_list);
		box->music_list = NULL;
		return CPYMO_ERR_OUT_OF_MEM;
	}

	box->music_title = (cpymo_backend_text *)(box->music_filename + box->music_count);
	for (uintptr_t i = 0; i < box->music_count; ++i)
		box->music_title[i] = NULL;

	uintptr_t i = 0;
	const cpymo_gameconfig *c = &e->gameconfig;
	box->font_size = c->fontsize / 240.0f * c->imagesize_h * 0.8f;

#ifdef ENABLE_TEXT_EXTRACT
	box->music_title_text = (char **)malloc(sizeof(char *) * box->music_count);
	cpymo_list_ui_set_selection_changed_callback(
		e, &cpymo_musicbox_visual_help_selection_change);
#endif

	do {
		cpymo_str music_file =
			cpymo_parser_curline_pop_commacell(&p);

		cpymo_str music_title =
			cpymo_parser_curline_pop_commacell(&p);

		cpymo_str_trim(&music_file);
		cpymo_str_trim(&music_title);

		if (music_file.len == 0) continue;

		box->music_filename[i] = music_file;

#ifdef ENABLE_TEXT_EXTRACT
		if (box->music_title_text) 
			box->music_title_text[i] = cpymo_str_copy_malloc(music_title);
#endif

		float width;
		error_t err = cpymo_backend_text_create(&box->music_title[i], &width, music_title, box->font_size);
		if (err != CPYMO_ERR_SUCC) {
			for (uintptr_t i = 0; i < box->music_count; ++i)
				if(box->music_title[i])
					cpymo_backend_text_free(box->music_title[i]);
			free(box->music_list);
			box->music_list = NULL;
			return err;
		}

		i++;
	} while (cpymo_parser_next_line(&p) && i < box->music_count);

	assert(i == box->music_count);
	cpymo_list_ui_set_current_node(e, ENCODE_NODE(0));

	return CPYMO_ERR_SUCC;
}


