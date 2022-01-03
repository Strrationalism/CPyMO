#include <3ds.h>
#include <stdio.h>
#include <cpymo_engine.h>
#include <citro3d.h>
#include <citro2d.h>
#include "select_game.h"

cpymo_engine engine;
C3D_RenderTarget *screen;

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

int main(void) {
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);

	const char *gamename = "DAICHYAN_s60v3";	//select_game();
	if (gamename == NULL) {
		gfxExit();
		return 0;
	}

	error_t err = cpymo_engine_init(&engine, "/pymogames/DAICHYAN_s60v3");
	if (err != CPYMO_ERR_SUCC) {
		printf("[Error] cpymo_engine_init: %s.", cpymo_error_message(err));
		gfxExit();
		return -1;
	}

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

	screen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	u32 clr = C2D_Color32(0x00, 0x00, 0xFF, 0xFF);
	u32 wtf = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	

	while (aptMainLoop()) {
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break;

		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(screen, clr);

			C2D_SceneBegin(screen);

			C2D_DrawRectangle(200 - 25, 120 - 25, -1.0f, 50, 50, wtf, wtf, wtf, wtf);

			C3D_FrameEnd(0);
		}

		gspWaitForVBlank();
	}

	C2D_Fini();
	C3D_Fini();

	gfxExit();

	return 0;
}
