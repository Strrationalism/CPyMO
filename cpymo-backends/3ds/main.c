#include <3ds.h>
#include <stdio.h>
#include <cpymo_gameconfig.h>

#include "select_game.h"

cpymo_gameconfig gameconfig;

int main(void) {
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	gfxSetDoubleBuffering(GFX_BOTTOM, false);

	const char *gamename = select_game();
	if (gamename == NULL) {
		gfxExit();
		return 0;
	}

	while (aptMainLoop()) {
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break;

		gfxFlushBuffers();
		gfxSwapBuffers();

		gspWaitForVBlank();
	}

	gfxExit();

	return 0;
}
