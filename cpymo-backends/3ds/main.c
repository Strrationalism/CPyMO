#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#include <cpymo_prelude.h>
#include <3ds.h>
#include <stdio.h>
#include <cpymo_engine.h>
#include <citro3d.h>
#include <citro2d.h>
#include <stdbool.h>
#include <libavutil/log.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#include <cpymo_backend_text.h>
#include <cpymo_backend_save.h>
#include <cpymo_game_selector.h>

cpymo_engine engine;
C3D_RenderTarget *screen1, *screen2, *screen3 = NULL;
float render_3d_offset;
bool fill_screen;

extern void cpymo_backend_image_init(float, float);

extern error_t cpymo_backend_text_sys_init();
extern void cpymo_backend_text_sys_free();

bool enhanced_3ds_display_mode = false;
bool drawing_bottom_screen;

bool enhanced_3ds_display_mode_touch_ui_enabled(void)
{
	if (!enhanced_3ds_display_mode) return false;
	if (engine.ui) return true;
	return false;
}

float enhanced_3ds_bottom_yoffset()
{
	if (enhanced_3ds_display_mode_touch_ui_enabled()) return 0;
	else {
		float msg_h = 0;
		
		if (engine.say.msgbox && engine.say.namebox) {
			float ratio = (float)engine.gameconfig.imagesize_w / (float)engine.say.msgbox_w;
			msg_h = (float)engine.say.msgbox_h * ratio;
		}
		else {
			msg_h = engine.gameconfig.imagesize_h * 0.25f;
		}
		
		float y = (float)engine.gameconfig.imagesize_h - msg_h;
		float fontsize = cpymo_gameconfig_font_size(&engine.gameconfig);
		float namebox_h = fontsize * 1.4f;
		float namebox_y = y - namebox_h;

		return -namebox_y + 16;		
	}
}

static void ensure_save_dir(const char *gamedir)
{
	if (R_FAILED(fsInit())) return;

	char *path = (char *)alloca(strlen(gamedir) + 8);
	strcpy(path, gamedir);
	strcat(path, "/save");

	FS_Archive archive;
	Result error = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC_WRITE_ONLY, fsMakePath(PATH_EMPTY, ""));
	if (R_FAILED(error)) {
		fsExit();
		return;
	}

	FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, path), FS_ATTRIBUTE_DIRECTORY);
	FSUSER_CloseArchive(archive);
	fsExit();
}

static void save_screen_mode()
{
	if (engine.assetloader.gamedir == NULL) return;
	FILE *sav = cpymo_backend_write_save(engine.assetloader.gamedir, "3ds-display-mode.csav");

	if (sav) {
		uint8_t save = fill_screen;
		save = save << 1;
		save = save | enhanced_3ds_display_mode;
		fwrite(&save, sizeof(save), 1, sav);
		fclose(sav);
	}
}

static void load_screen_mode(const char *gamedir)
{
	FILE *sav = cpymo_backend_read_save(gamedir, "3ds-display-mode.csav");

	if (sav) {
		uint8_t save;

		if (fread(&save, sizeof(save), 1, sav) == 1) {
			bool enhanced = save & 1;
			save = save >> 1;
			fill_screen = save;

			if (enhanced_3ds_display_mode == true) {
				if (enhanced == false)
					enhanced_3ds_display_mode = false;
			}
		}

		fclose(sav);
	}
}

static cpymo_game_selector_item *load_game_list()
{
	if (R_FAILED(fsInit())) return NULL;

	FS_Archive archive;
	Result err = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));

	if (R_FAILED(err)) {
		fsExit();
		return NULL;
	}

	Handle handle;
	err = FSUSER_OpenDirectory(&handle, archive, fsMakePath(PATH_ASCII, "/pymogames/"));
	if (R_FAILED(err)) {
		FSUSER_CloseArchive(archive);
		fsExit();
		return NULL;
	}

	cpymo_game_selector_item *items = NULL, *tail = NULL;

	u32 result = 0;
	do {
		FS_DirectoryEntry item;
		err = FSDIR_Read(handle, &result, 1, &item);

		if (R_FAILED(err) || result != 1 || (item.attributes & FS_ATTRIBUTE_DIRECTORY) == 0) 
			continue;

		size_t name_len = 0;
		for (size_t i = 0; i < sizeof(item.name) / sizeof(item.name[0]); ++i) {
			if (item.name[i] == 0) break;
			++name_len;
		}

		char *path = (char *)malloc(name_len + 16);
		if (path == NULL) continue;

		strcpy(path, "/pymogames/");
		for (size_t i = 0; i < name_len; ++i)
			path[i + 11] = (char)item.name[i];
		path[name_len + 11] = '\0';

		cpymo_game_selector_item *cur = NULL;
		error_t err = cpymo_game_selector_item_create(&cur, &path);
		if (err != CPYMO_ERR_SUCC) {
			free(path);
			continue;
		}

		if (items == NULL) {
			items = cur;
			tail = cur;
		}
		else {
			tail->next = cur;
			tail = cur;
		}
	} while(result);

	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	fsExit();

	return items;
}

static char *get_last_selected_game_dir()
{
	char *str = NULL;
	size_t len;
	error_t err = cpymo_utils_loadfile("/pymogames/last_game.txt", &str, &len);

	if (err != CPYMO_ERR_SUCC) return NULL;

	char *ret = realloc(str, len + 1);
	if (ret == NULL) {
		free(str);
		return NULL;
	}

	ret[len] = '\0';
	return ret;
}

static error_t before_select_game(cpymo_engine *e, const char *gamedir)
{
	size_t len = strlen(gamedir);
	FILE *f = fopen("/pymogames/last_game.txt", "wb");
	if (f) {
		fwrite(gamedir, len, 1, f);
		fclose(f);
	}

	ensure_save_dir(gamedir);

	enhanced_3ds_display_mode = true;

	hidScanInput();
	if (hidKeysHeld() & KEY_L) {
		consoleInit(GFX_BOTTOM, NULL);
		gfxSetDoubleBuffering(GFX_BOTTOM, false);

		enhanced_3ds_display_mode = false;
	}

	if(enhanced_3ds_display_mode) {
		screen3 = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
		load_screen_mode(gamedir);
	}

	if (screen3 == NULL) 
		enhanced_3ds_display_mode = false;

	return CPYMO_ERR_SUCC;
}

static error_t after_select_game(cpymo_engine *e, const char *gamedir)
{
	cpymo_backend_image_init(e->gameconfig.imagesize_w, e->gameconfig.imagesize_h);
	return CPYMO_ERR_SUCC;
}

int main(void) {
	srand((unsigned)time(NULL));
	engine.ui = NULL;
	engine.say.active = false;
	bool is_new_3ds = false;
	APT_CheckNew3DS(&is_new_3ds);

	av_log_set_level(AV_LOG_ERROR);
	
	fill_screen = false;
	
	gfxInitDefault();

	gfxSet3D(true);

	if(is_new_3ds) {
		osSetSpeedupEnable(true);
	}

	engine.audio.enabled = false;

	if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
		gfxExit();
		return 0;
	}

	if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
		C3D_Fini();
		gfxExit();
		return 0;
	}

	C2D_Prepare();

	screen1 = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	if(screen1 == NULL) {
		C2D_Fini();
		C3D_Fini();
		gfxExit();
		return 0;
	}

	screen2 = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	if(screen2 == NULL) {
		C3D_RenderTargetDelete(screen1);
		C2D_Fini();
		C3D_Fini();
		gfxExit();
		return 0;
	}

	if(cpymo_backend_text_sys_init() != CPYMO_ERR_SUCC) {
		C3D_RenderTargetDelete(screen1);
		C3D_RenderTargetDelete(screen2);
		C2D_Fini();
		C3D_Fini();
		gfxExit();
		return 0;
	}

	cpymo_backend_image_init(400, 240);

	extern void cpymo_backend_audio_init();
	cpymo_backend_audio_init();
	
	cpymo_game_selector_item *item = load_game_list();
	char *last_select_game = get_last_selected_game_dir();
	error_t err = cpymo_engine_init_with_game_selector(
		&engine, 400, 240, 22, 20, 3, &item, &before_select_game, &after_select_game, &last_select_game);
	
	if (err != CPYMO_ERR_SUCC) {
		printf("[Error] cpymo_engine_init_with_game_selector: %s.\n", cpymo_error_message(err));
		if (last_select_game) free(last_select_game);
		cpymo_game_selector_item_free_all(item);
		cpymo_backend_text_sys_free();
		C3D_RenderTargetDelete(screen1);
		C3D_RenderTargetDelete(screen2);
		C2D_Fini();
		C3D_Fini();
		gfxExit();
		return 0;
	}

	const u32 clr = C2D_Color32(0, 0, 0, 0);

	float prevSlider = NAN;
	TickCounter tickCounter;
	osTickCounterStart(&tickCounter);

	void cpymo_backend_audio_unlock(void);
	cpymo_backend_audio_unlock();

	while (aptMainLoop()) {
		if(aptShouldClose()) break;

		hidScanInput();

		bool redraw = false;

		osTickCounterUpdate(&tickCounter);
		double deltaTime = osTickCounterRead(&tickCounter) / 1000.0;
		err = cpymo_engine_update(&engine, (float)deltaTime, &redraw);
		switch(err) {
		case CPYMO_ERR_NO_MORE_CONTENT: goto EXIT;
		case CPYMO_ERR_SUCC: break;
		default: {
			printf("[Error] %s(%d).\n", cpymo_error_message(err), err);
			while(aptMainLoop()) {
				hidScanInput();
				gspWaitForVBlank();

				u32 keys = hidKeysHeld();
				if((keys & KEY_ZL)) goto EXIT;
			}
		}
		}

		float slider = osGet3DSliderState();
		if(slider != prevSlider) {
			redraw = true;
			prevSlider = slider;
		}

		if (hidKeysDown() & KEY_SELECT) {
			redraw = true;
			if (screen3) {
				enhanced_3ds_display_mode = !enhanced_3ds_display_mode;
				if (!enhanced_3ds_display_mode) fill_screen = !fill_screen;
			}
			else {
				fill_screen = !fill_screen;
			}
		}

		if(redraw || true) {
			drawing_bottom_screen = false;
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(screen1, clr);

			C2D_SceneBegin(screen1);
			render_3d_offset = slider;
			cpymo_engine_draw(&engine);

			if(slider > 0){
				C2D_TargetClear(screen2, clr);
				C2D_SceneBegin(screen2);
				render_3d_offset = -slider;
				cpymo_engine_draw(&engine);
			}

			if(screen3) {
				C2D_SceneBegin(screen3);
				C2D_TargetClear(screen3, clr);

				if(enhanced_3ds_display_mode) {
					render_3d_offset = 0;
					drawing_bottom_screen = true;
					cpymo_engine_draw(&engine);
				}
			}

			C3D_FrameEnd(0);
		} else {
			gspWaitForVBlank();
		}

		if(hidKeysDown() & KEY_START) break;
	}

	EXIT:

	save_screen_mode();

	cpymo_backend_text_sys_free();
	if(screen3) C3D_RenderTargetDelete(screen3);
	C3D_RenderTargetDelete(screen2);
	C3D_RenderTargetDelete(screen1);
	cpymo_engine_free(&engine);

	extern void cpymo_backend_audio_free();
	cpymo_backend_audio_free();
	
	C2D_Fini();
	C3D_Fini();

	gfxExit();

	return 0;
}

