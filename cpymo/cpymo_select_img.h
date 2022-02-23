#ifndef INCLUDE_CPYMO_SELECT_IMG
#define INCLUDE_CPYMO_SELECT_IMG

#include <stdbool.h>
#include <cpymo_backend_image.h>
#include <cpymo_backend_text.h>
#include "cpymo_parser.h"
#include "cpymo_tween.h"
#include "cpymo_assetloader.h"
#include "cpymo_wait.h"
#include "cpymo_hash_flags.h"
#include "cpymo_key_pulse.h"

struct cpymo_engine;

enum cpymo_select_img_selection_hint_state {
	cpymo_select_img_selection_nohint = 0,
	cpymo_select_img_selection_hint01,
	cpymo_select_img_selection_hint23
};

struct cpymo_select_img;
typedef error_t (*cpymo_select_img_ok_callback)(struct cpymo_engine *, int, uint64_t, struct cpymo_select_img *);

typedef struct {
	cpymo_backend_image image;
	cpymo_backend_text or_text;
	cpymo_color text_color;
	float x, y;
	int w, h;
	bool enabled : 1;

	uint64_t hash;
	bool has_selected;

	enum cpymo_select_img_selection_hint_state hint_state : 3;
} cpymo_select_img_selection;

struct cpymo_select_img {
	cpymo_select_img_selection *selections;

	cpymo_backend_image select_img_image;
	int select_img_image_w, select_img_image_h;

	int current_selection;
	size_t all_selections;
	bool save_enabled;

	cpymo_backend_image sel_highlight;
	int sel_highlight_w, sel_highlight_h;

	bool show_option_background;
	cpymo_backend_image option_background;
	int option_background_w, option_background_h;

	cpymo_key_pluse key_up, key_down;

	cpymo_backend_image hint[4];
	int hint_w[4], hint_h[4];
	float hint_timer;
	bool hint_tiktok;

	cpymo_select_img_ok_callback ok_callback;
};

typedef struct cpymo_select_img cpymo_select_img;

void cpymo_select_img_reset(cpymo_select_img *img);

error_t cpymo_select_img_configuare_begin(
	cpymo_select_img *sel, size_t selections,
	cpymo_parser_stream_span image_name_or_empty_when_select_imgs,
	cpymo_assetloader *loader, cpymo_gameconfig *gameconfig);

static inline void cpymo_select_img_configuare_set_ok_callback(cpymo_select_img *sel, cpymo_select_img_ok_callback ok) {
	sel->ok_callback = ok;
}

void cpymo_select_img_configuare_select_img_selection(
	struct cpymo_engine *engine, float x, float y, bool enabled, uint64_t hash);

error_t cpymo_select_img_configuare_select_imgs_selection(
	struct cpymo_engine *engine, cpymo_parser_stream_span image_name, float x, float y, bool enabled, uint64_t hash);

void cpymo_select_img_configuare_end(
	cpymo_select_img *sel, cpymo_wait *wait,
	struct cpymo_engine *engine, int init_position);

error_t cpymo_select_img_update(struct cpymo_engine *engine, cpymo_select_img *o, float dt);
void cpymo_select_img_draw(const cpymo_select_img *, int logical_screen_w, int logical_screen_h, bool gray_selected);

static inline void cpymo_select_img_init(cpymo_select_img *select_img)
{
	select_img->selections = NULL;
	select_img->select_img_image = NULL;
	select_img->sel_highlight = NULL;
	select_img->option_background = NULL;
	select_img->show_option_background = false;

	cpymo_key_pluse_init(&select_img->key_up, false);
	cpymo_key_pluse_init(&select_img->key_down, false);

	for (size_t i = 0; i < 4; ++i) select_img->hint[i] = NULL;
}

static inline void cpymo_select_img_free(cpymo_select_img *img)
{
	cpymo_select_img_reset(img);
	if (img->sel_highlight) cpymo_backend_image_free(img->sel_highlight);
	if (img->option_background) cpymo_backend_image_free(img->option_background);
}

error_t cpymo_select_img_configuare_select_text(
	cpymo_select_img *sel, cpymo_assetloader *loader, cpymo_gameconfig *gc, cpymo_hash_flags *flags,
	cpymo_parser_stream_span text, bool enabled, 
	enum cpymo_select_img_selection_hint_state hint_mode,
	uint64_t hash);

void cpymo_select_img_configuare_select_text_hint_pic(
	struct cpymo_engine *engine, cpymo_parser_stream_span hint);

void cpymo_select_img_configuare_end_select_text(
	cpymo_select_img *sel,
	cpymo_wait *waiter,
	struct cpymo_engine *engine, 
	float x1, float y1, 
	float x2, float y2, 
	cpymo_color col, 
	int init_pos, 
	bool show_option_background);

#endif