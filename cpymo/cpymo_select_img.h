#ifndef INCLUDE_CPYMO_SELECT_IMG
#define INCLUDE_CPYMO_SELECT_IMG

#include <stdbool.h>
#include <cpymo_backend_image.h>
#include <cpymo_backend_text.h>
#include "cpymo_parser.h"
#include "cpymo_tween.h"
#include "cpymo_assetloader.h"

struct cpymo_engine;

typedef struct {
	cpymo_backend_image image;
	cpymo_backend_text or_text;
	cpymo_color text_color;
	float x, y;
	int w, h;
	bool enabled;
} cpymo_select_img_selection;

typedef struct {
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
} cpymo_select_img;

void cpymo_select_img_reset(cpymo_select_img *img);

error_t cpymo_select_img_configuare_begin(
	struct cpymo_engine *engine, size_t selections,
	cpymo_parser_stream_span image_name_or_empty_when_select_imgs);

void cpymo_select_img_configuare_select_img_selection(
	struct cpymo_engine *engine, float x, float y, bool enabled);

error_t cpymo_select_img_configuare_select_imgs_selection(
	struct cpymo_engine *engine, cpymo_parser_stream_span image_name, float x, float y, bool enabled);

void cpymo_select_img_configuare_end(struct cpymo_engine *engine, int init_position);

error_t cpymo_select_img_update(struct cpymo_engine *engine);
void cpymo_select_img_draw(const cpymo_select_img *, int logical_screen_w, int logical_screen_h);

static inline void cpymo_select_img_init(cpymo_select_img *select_img)
{
	select_img->selections = NULL;
	select_img->select_img_image = NULL;
	select_img->sel_highlight = NULL;
	select_img->option_background = NULL;
	select_img->show_option_background = false;
}

static inline void cpymo_select_img_free(cpymo_select_img *img)
{
	cpymo_select_img_reset(img);
	if (img->sel_highlight) cpymo_backend_image_free(img->sel_highlight);
	if (img->option_background) cpymo_backend_image_free(img->option_background);
}

error_t cpymo_select_img_configuare_select_text(
	struct cpymo_engine *engine, cpymo_parser_stream_span text, bool enabled);

void cpymo_select_img_configuare_end_select_text(
	struct cpymo_engine *engine, 
	float x1, float y1, 
	float x2, float y2, 
	cpymo_color col, 
	int init_pos, 
	bool show_option_background);

#endif