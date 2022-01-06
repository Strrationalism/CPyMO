#include <3ds.h>
#include <stdio.h>
#include <cpymo_engine.h>
#include <citro3d.h>
#include <citro2d.h>
#include "select_game.h"
#include <stdbool.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

cpymo_engine engine;
C3D_RenderTarget *screen1, *screen2;
float render_3d_offset;
bool fill_screen;

extern void cpymo_backend_image_init(float, float);
extern void cpymo_backend_image_fill_screen_edges();

int main(void) {
	bool is_new_3ds = false;
	APT_CheckNew3DS(&is_new_3ds);
	
	fill_screen = false;
	
	gfxInitDefault();
	gfxSet3D(true);
	consoleInit(GFX_BOTTOM, NULL);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);

	if(is_new_3ds) {
		printf("[Info] New3DS\n");
		osSetSpeedupEnable(true);
	}

	/*const char *gamename = "DAICHYAN_s60v3";	//select_game();
	if (gamename == NULL) {
		gfxExit();
		return 0;
	}*/

	error_t err = cpymo_engine_init(&engine, "/pymogames/MO1_android");
	if (err != CPYMO_ERR_SUCC) {
		printf("[Error] cpymo_engine_init: %s.", cpymo_error_message(err));
		gfxExit();
		return -1;
	}

	cpymo_backend_image_init(engine.gameconfig.imagesize_w, engine.gameconfig.imagesize_h);

	if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
		cpymo_engine_free(&engine);
		gfxExit();
		return -1;
	}

	if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
		C3D_Fini();
		cpymo_engine_free(&engine);
		gfxExit();
		return -1;
	}

	C2D_Prepare();

	screen1 = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	screen2 = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);

	const u32 clr = C2D_Color32(0, 0, 0, 0);

	float prevSlider = NAN;
	TickCounter tickCounter;
	osTickCounterStart(&tickCounter);
	while (aptMainLoop()) {
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
			while(1) gspWaitForVBlank();
		}
		}

		float slider = osGet3DSliderState();
		if(slider != prevSlider) {
			redraw = true;
			prevSlider = slider;
		}

		if (hidKeysDown() & KEY_SELECT) {
			redraw = true;
			fill_screen = !fill_screen;
		}

		if(redraw) {
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(screen1, clr);
			C2D_TargetClear(screen2, clr);

			C2D_SceneBegin(screen1);
			render_3d_offset = slider;
			cpymo_engine_draw(&engine);
			cpymo_backend_image_fill_screen_edges();

			if(slider > 0){
				C2D_SceneBegin(screen2);
				render_3d_offset = -slider;
				cpymo_engine_draw(&engine);
				cpymo_backend_image_fill_screen_edges();
			}

			C3D_FrameEnd(0);
		} else {
			gspWaitForVBlank();
			gspWaitForVBlank();
			gspWaitForVBlank();
		}
	}

	EXIT:
	cpymo_engine_free(&engine);
	
	C2D_Fini();
	C3D_Fini();

	gfxExit();

	return 0;
}
