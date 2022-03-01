#include <3ds.h>
#include <stdio.h>
#include <cpymo_engine.h>
#include <citro3d.h>
#include <citro2d.h>
#include "select_game.h"
#include <stdbool.h>
#include <libavutil/log.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cpymo_backend_text.h>
#include "cpymo_backend_save.h"

cpymo_engine engine;
C3D_RenderTarget *screen1, *screen2, *screen3 = NULL;
float render_3d_offset;
bool fill_screen;

extern void cpymo_backend_image_init(float, float);
extern void cpymo_backend_image_fill_screen_edges();

extern error_t cpymo_backend_text_sys_init();
extern void cpymo_backend_text_sys_free();

extern const bool cpymo_input_fast_kill_pressed;

bool enhanced_3ds_display_mode = true;
bool drawing_bottom_screen;

float enhanced_3ds_bottom_yoffset()
{
	if(engine.say.msgbox && engine.say.namebox) {
		float ratio = (float)engine.gameconfig.imagesize_w / (float)engine.say.msgbox_w;
		float msg_h = (float)engine.say.msgbox_h * ratio;
		float y = (float)engine.gameconfig.imagesize_h - msg_h;
		float fontsize = cpymo_gameconfig_font_size(&engine.gameconfig);
		float namebox_h = fontsize * 1.4f;
		float namebox_y = y - namebox_h;
		
		return -namebox_y + 16;
	}
	else {
		return 0;
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
	FILE *sav = cpymo_backend_write_save(engine.assetloader.gamedir, "3ds-display-mode.csav");

	if (sav) {
		uint8_t save = fill_screen;
		save = save << 1;
		save = save | enhanced_3ds_display_mode;
		fwrite(&save, sizeof(save), 1, sav);
		fclose(sav);
	}
}

static void load_screen_mode()
{
	FILE *sav = cpymo_backend_read_save(engine.assetloader.gamedir, "3ds-display-mode.csav");

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

int main(void) {
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
	char *gamedir = select_game();
	if (gamedir == NULL) {
		cpymo_backend_text_sys_free();
		C3D_RenderTargetDelete(screen1);
		C3D_RenderTargetDelete(screen2);
		C2D_Fini();
		C3D_Fini();
		gfxExit();
		return 0;
	}

	if(!enhanced_3ds_display_mode) {
		consoleInit(GFX_BOTTOM, NULL);
		gfxSetDoubleBuffering(GFX_BOTTOM, false);
	}

	extern void cpymo_backend_audio_init();
	cpymo_backend_audio_init();

	ensure_save_dir(gamedir);
	error_t err = cpymo_engine_init(&engine, gamedir);
	free(gamedir);
	if (err != CPYMO_ERR_SUCC) {
		printf("[Error] cpymo_engine_init: %s.", cpymo_error_message(err));
		cpymo_backend_text_sys_free();
		C3D_RenderTargetDelete(screen1);
		C3D_RenderTargetDelete(screen2);
		C2D_Fini();
		C3D_Fini();
		gfxExit();
		return 0;
	}

	load_screen_mode();

	cpymo_backend_image_init(engine.gameconfig.imagesize_w, engine.gameconfig.imagesize_h);

	if(enhanced_3ds_display_mode) {
		screen3 = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
		if(screen3 == NULL) {
			cpymo_engine_free(&engine);
			C3D_RenderTargetDelete(screen1);
			C3D_RenderTargetDelete(screen2);
			C2D_Fini();
			C3D_Fini();
			gfxExit();
			return 0;
		}
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
			printf("[Error] %s.\n", cpymo_error_message(err));
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

		if(hidKeysDown() & KEY_SELECT) {
			redraw = true;
			fill_screen = !fill_screen;
		}

		if((hidKeysDown() & KEY_START) && screen3 != NULL) {
			redraw = true;
			enhanced_3ds_display_mode = !enhanced_3ds_display_mode;
		}

		if(redraw) {
			drawing_bottom_screen = false;
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(screen1, clr);

			C2D_SceneBegin(screen1);
			render_3d_offset = slider;
			cpymo_engine_draw(&engine);
			cpymo_backend_image_fill_screen_edges();

			if(slider > 0){
				C2D_TargetClear(screen2, clr);
				C2D_SceneBegin(screen2);
				render_3d_offset = -slider;
				cpymo_engine_draw(&engine);
				cpymo_backend_image_fill_screen_edges();
			}

			if(screen3) {
				C2D_SceneBegin(screen3);
				C2D_TargetClear(screen3, clr);

				if(enhanced_3ds_display_mode) {
					render_3d_offset = 0;
					drawing_bottom_screen = true;
					cpymo_engine_draw(&engine);
					cpymo_backend_image_fill_screen_edges();
				}
			}

			C3D_FrameEnd(0);
		} else {
			gspWaitForVBlank();
		}

		if(cpymo_input_fast_kill_pressed) break;
	}

	EXIT:

	save_screen_mode();

	cpymo_backend_text_sys_free();
	if(enhanced_3ds_display_mode) C3D_RenderTargetDelete(screen3);
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
