#include "select_game.h"
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <stdio.h>
#include <cpymo_error.h>
#include "cpymo_backend_image.h"
#include "cpymo_backend_text.h"
#include <cpymo_gameconfig.h>
#include <stb_image.h>
#include "cpymo_backend_input.h"

extern C3D_RenderTarget *screen1, *screen2;
extern float render_3d_offset;
extern bool drawing_bottom_screen;
extern const bool cpymo_input_fast_kill_pressed;

typedef struct {
	float name_width;
	cpymo_backend_text name;
	char *path;
	int icon_w, icon_h;
	cpymo_backend_image icon;
} game_info;

#define GAME_INFO_FONT_SIZE 22
#define GAME_ICON_SIZE 64
#define ITEM_HEIGHT 80
#define ITEMS_PER_SCREEN 3

static void free_game_info(game_info *g) 
{
	if (g->name) cpymo_backend_text_free(g->name);
	if (g->path) free(g->path);
	if (g->icon) cpymo_backend_image_free(g->icon);

	g->icon = NULL;
	g->name = NULL;
	g->path = NULL;
}

static void free_all_game_info(game_info *info, size_t count)
{
	for (size_t i = 0; i < count; i++) 
		free_game_info(info + i);

	free(info);
}


static error_t load_game_info(game_info *out, char **gamedir_move_in) 
{
	out->icon = NULL;
	out->name = NULL;
	out->path = NULL;
	char *path = alloca(strlen(*gamedir_move_in) + 16);
	sprintf(path, "%s/gameconfig.txt", *gamedir_move_in);

	cpymo_gameconfig gameconfig;
	error_t err = cpymo_gameconfig_parse_from_file(&gameconfig, path);
	CPYMO_THROW(err);

	sprintf(path, "%s/icon.png", *gamedir_move_in);
	stbi_uc *px = stbi_load(path, &out->icon_w, &out->icon_h, NULL, 4);
	if (px) {
		err = cpymo_backend_image_load(
			&out->icon,
			px,
			out->icon_w,
			out->icon_h,
			cpymo_backend_image_format_rgba);
		
		if (err != CPYMO_ERR_SUCC) {
			out->icon = NULL;
			out->icon_h = 0;
			out->icon_w = 0;
		}
	}

	out->path = *gamedir_move_in;
	*gamedir_move_in = NULL;

	err = cpymo_backend_text_create(
		&out->name,
		&out->name_width,
		cpymo_parser_stream_span_pure(gameconfig.gametitle),
		GAME_INFO_FONT_SIZE);

	if (err != CPYMO_ERR_SUCC) {
		out->name = NULL;
		free(out);
		return err;
	}

	return CPYMO_ERR_SUCC;
}

typedef struct {
	size_t game_count;
	game_info *games;
	cpymo_input prev_input;

	cpymo_backend_text hint1, hint2;

	char *ok;

	bool redraw;
} select_game_ui;

static void draw_game_info(const game_info *info, float y, bool selected) 
{
	if (selected) {
		float xywh[] = { 0, y, 400, ITEM_HEIGHT };
		cpymo_backend_image_fill_rects(xywh, 1, cpymo_color_white, 0.5f, 
			cpymo_backend_image_draw_type_ui_element);
	}

	if (info->icon) {
		cpymo_backend_image_draw(16, y + 6, GAME_ICON_SIZE, GAME_ICON_SIZE, info->icon, 0, 0, 
			info->icon_w, info->icon_h, 1.0f, cpymo_backend_image_draw_type_ui_element);
	}

	cpymo_backend_text_draw(info->name, 76 + 16, y + GAME_INFO_FONT_SIZE, cpymo_color_white, 1.0f,
		cpymo_backend_image_draw_type_ui_element);
}

static void draw_select_game(const select_game_ui *ui) 
{
	if (ui->hint1) {
		cpymo_backend_text_draw(
			ui->hint1, 127, 100, cpymo_color_white, 0.5f,
			cpymo_backend_image_draw_type_ui_element);

		cpymo_backend_text_draw(
			ui->hint2, 20, 130, cpymo_color_white, 0.25f,
			cpymo_backend_image_draw_type_ui_element);
	}
	else {
		for (size_t i = 0; i < ITEMS_PER_SCREEN; ++i)
			draw_game_info(ui->games, i * ITEM_HEIGHT, i == 1);
	}
}

static void select_game_ok(select_game_ui *ui, game_info *info)
{
	ui->ok = info->path;
	info->path = NULL;
}

static void update_select_game(select_game_ui *ui) 
{
	if (ui->hint1) return;

	cpymo_input input = cpymo_input_snapshot();

	if (input.ok) {
		select_game_ok(ui, ui->games);
		return;
	}

	ui->prev_input = input;
}

char * select_game()
{
	select_game_ui ui;
	ui.redraw = true;
	ui.games = malloc(sizeof(game_info));
	ui.game_count = 1;
	ui.ok = NULL;
	ui.hint1 = NULL;
	ui.hint2 = NULL;

	char *str = malloc(32);
	strcpy(str, "/pymogames/mashiro_android");
	load_game_info(ui.games, &str);

	if (ui.game_count == 0 || ui.games == NULL) {
		float _;
		const char *hint1_msg = "No games found.";
		const char *hint2_msg =
			"Please make sure folder\"pymogames\" is in SD card root,\n"
			"and that you have at least one game in it.\n";

		error_t err = cpymo_backend_text_create(&ui.hint1, &_, 
			cpymo_parser_stream_span_pure(hint1_msg), GAME_INFO_FONT_SIZE);
		if (err != CPYMO_ERR_SUCC) return NULL;

		err = cpymo_backend_text_create(&ui.hint2, &_, 
			cpymo_parser_stream_span_pure(hint2_msg), GAME_INFO_FONT_SIZE / 1.3f);
		if (err != CPYMO_ERR_SUCC) {
			cpymo_backend_text_free(ui.hint1);
			return NULL;
		}
	}
	
	drawing_bottom_screen = false;
	float prevSlider = NAN;
	while (aptMainLoop() && !cpymo_input_fast_kill_pressed) {
		hidScanInput();

		update_select_game(&ui);
		if (ui.ok) break;

		float slider = osGet3DSliderState();
		if (ui.redraw || prevSlider != slider) {
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(screen1, C2D_Color32(0, 0, 0, 0));
			C2D_SceneBegin(screen1);
			render_3d_offset = slider;
			draw_select_game(&ui);

			if (slider > 0) {
				C2D_TargetClear(screen2, C2D_Color32(0, 0, 0, 0));
				C2D_SceneBegin(screen2);
				render_3d_offset = -slider;
				draw_select_game(&ui);
			}

			C3D_FrameEnd(0);
			ui.redraw = false;
		}

		gspWaitForVBlank();
	}

	free_all_game_info(ui.games, ui.game_count);
	if (ui.hint1) cpymo_backend_text_free(ui.hint1);
	if (ui.hint2) cpymo_backend_text_free(ui.hint2);

	return ui.ok;
}
