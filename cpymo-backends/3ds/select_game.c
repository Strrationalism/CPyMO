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
#include <cpymo_key_pulse.h>
#include <cpymo_utils.h>

extern C3D_RenderTarget *screen1, *screen2;
extern float render_3d_offset;
extern bool drawing_bottom_screen;
extern const bool cpymo_input_fast_kill_pressed;
extern bool enhanced_3ds_display_mode;

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

	size_t current_screen_first_game_id;
	size_t selected_game_rel_to_cur_first;

	cpymo_backend_text hint1, hint2;
	cpymo_key_pluse key_up, key_down;

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
			ui->hint1, 145, 100, cpymo_color_white, 0.5f,
			cpymo_backend_image_draw_type_ui_element);

		cpymo_backend_text_draw(
			ui->hint2, 20, 130, cpymo_color_white, 0.25f,
			cpymo_backend_image_draw_type_ui_element);
	}
	else {
		int current_selected = ui->current_screen_first_game_id + ui->selected_game_rel_to_cur_first;
		for (size_t i = 0; i < ITEMS_PER_SCREEN; ++i)
			if (ui->current_screen_first_game_id + i < ui->game_count)
				draw_game_info(&ui->games[ui->current_screen_first_game_id + i], i * ITEM_HEIGHT, 
					current_selected == ui->current_screen_first_game_id + i);
	}
}

static void select_game_ok(select_game_ui *ui, game_info *info)
{
	ui->ok = info->path;
	info->path = NULL;

	FILE *last_game_record = fopen("/pymogames/last_game.txt", "wb");
	if (last_game_record) {
		fwrite(ui->ok, strlen(ui->ok), 1, last_game_record);
		fclose(last_game_record);
	}
}

static void update_select_game(select_game_ui *ui) 
{
	if (ui->hint1) return;

	cpymo_input input = cpymo_input_snapshot();

	size_t cursel = ui->current_screen_first_game_id + ui->selected_game_rel_to_cur_first;

	if (input.ok) {
		if (input.skip) 
			enhanced_3ds_display_mode = false;
		
		select_game_ok(ui, ui->games + cursel);
		return;
	}

	cpymo_key_pluse_update(&ui->key_up, 0.016f, input.up);
	cpymo_key_pluse_update(&ui->key_down, 0.016f, input.down);

	if (cpymo_key_pluse_output(&ui->key_down)) {
		if (cursel + 1 < ui->game_count) {
			ui->selected_game_rel_to_cur_first++;
			if (ui->selected_game_rel_to_cur_first >= ITEMS_PER_SCREEN) {
				ui->current_screen_first_game_id++;
				ui->selected_game_rel_to_cur_first--;
				ui->redraw = true;
			}
		}
	}
	else if (cpymo_key_pluse_output(&ui->key_up)) {
		if (cursel > 0) {
			if (ui->selected_game_rel_to_cur_first > 0) {
				ui->selected_game_rel_to_cur_first--;
			}
			else {
				ui->current_screen_first_game_id--;
			}

			ui->redraw = true;
		}
	}

	ui->prev_input = input;
}

static error_t load_game_list(select_game_ui *ui)
{
	if (R_FAILED(fsInit())) return CPYMO_ERR_UNKNOWN;

	FS_Archive archive;
	Result err = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (R_FAILED(err)) {
		fsExit();
		return CPYMO_ERR_UNKNOWN;
	}

	Handle handle;
	err = FSUSER_OpenDirectory(&handle, archive, fsMakePath(PATH_ASCII, "/pymogames/"));
	if (R_FAILED(err)) {
		FSUSER_CloseArchive(archive);
		fsExit();
		return CPYMO_ERR_UNKNOWN;
	}

	cpymo_parser_stream_span last_loaded_game;
	last_loaded_game.begin = NULL;
	last_loaded_game.len = 0;

	error_t err2 = cpymo_utils_loadfile(
		"/pymogames/last_game.txt", (char **)&last_loaded_game.begin, &last_loaded_game.len);
	if (err2 != CPYMO_ERR_SUCC) {
		last_loaded_game.begin = NULL;
		last_loaded_game.len = 0;
	}

	size_t buf_size = 0;

	u32 result = 0;
	do {
		FS_DirectoryEntry item;
		err = FSDIR_Read(handle, &result, 1, &item);
		if (R_FAILED(err)) continue;
		if (result != 1) continue;
		if ((item.attributes & FS_ATTRIBUTE_DIRECTORY) == 0) continue;

		size_t name_len = 0;
		for (size_t i = 0; i < sizeof(item.name) / sizeof(item.name[0]); ++i) {
			if (item.name[i] == 0) break;
			name_len++;
		}

		char *path = (char *)malloc(name_len + 12);
		if (path == NULL) continue;

		strcpy(path, "/pymogames/");
		for (size_t i = 0; i < name_len; ++i)
			path[i + 11] = (char)item.name[i];
		path[name_len + 11] = '\0';

		game_info info;
		error_t err2 = load_game_info(&info, &path);

		if (err2 == CPYMO_ERR_SUCC) {
			if (ui->game_count + 1 > buf_size) {
				buf_size = (buf_size + 1) * 2;
				game_info *games = (game_info *)realloc(ui->games, buf_size * sizeof(game_info));

				if (games == NULL) {
					free_game_info(&info);
					if (last_loaded_game.begin) free((char *)last_loaded_game.begin);
					return CPYMO_ERR_SUCC;
				}

				ui->games = games;
			}

			if (cpymo_parser_stream_span_equals_str(last_loaded_game, info.path)) {
				ui->current_screen_first_game_id = ui->game_count;
				ui->selected_game_rel_to_cur_first = 0;

				if (ui->current_screen_first_game_id > 0) {
					ui->current_screen_first_game_id--;
					ui->selected_game_rel_to_cur_first++;
				}
			}

			ui->games[ui->game_count++] = info;
		}

	} while(result);

	if (last_loaded_game.begin) free((char *)last_loaded_game.begin);

	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	fsExit();

	return CPYMO_ERR_SUCC;
}

char * select_game()
{
	select_game_ui ui;
	ui.redraw = true;
	ui.games = NULL;
	ui.game_count = 0;
	ui.ok = NULL;
	ui.hint1 = NULL;
	ui.hint2 = NULL;
	ui.current_screen_first_game_id = 0;
	ui.selected_game_rel_to_cur_first = 0;

	cpymo_key_pluse_init(&ui.key_down, false);
	cpymo_key_pluse_init(&ui.key_up, false);

	error_t err = load_game_list(&ui);
	if (err != CPYMO_ERR_SUCC) return NULL;

	if (ui.game_count == 0 || ui.games == NULL) {
		float _;
		const char *hint1_msg = "未找到游戏";
		const char *hint2_msg =
			"你需要将游戏放在SD卡中的\"pymogames\"文件夹下，\n"
			"并确保路径中只包含英文、数字和下划线。\n";

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

	if (ui.games) free_all_game_info(ui.games, ui.game_count);
	if (ui.hint1) cpymo_backend_text_free(ui.hint1);
	if (ui.hint2) cpymo_backend_text_free(ui.hint2);

	return ui.ok;
}
