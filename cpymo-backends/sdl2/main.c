#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>
#include <cpymo_engine.h>
#include <cpymo_parser.h>
#include <cpymo_assetloader.h>
#include <string.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

cpymo_gameconfig gameconfig;
cpymo_assetloader assetloader;
SDL_Window *window;
SDL_Renderer *renderer;
cpymo_engine engine;

static void set_window_icon(const char *gamedir) 
{
	int w, h, channel;
	char icon_path[4096];

	sprintf(icon_path, "%s/icon.png", gamedir);

	stbi_uc *icon = stbi_load(icon_path, &w, &h, &channel, 4);
	if (icon == NULL) return;

	SDL_Surface *surface =
		SDL_CreateRGBSurfaceFrom(icon, w, h, channel * 8, channel * w, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	if (surface == NULL) {
		stbi_image_free(icon);
		return;
	}

	SDL_SetWindowIcon(window, surface);
	SDL_FreeSurface(surface);
	stbi_image_free(icon);
}

static void load_gameconfig(const char *gamedir) 
{
	char buf[4096];
	sprintf(buf, "%s/gameconfig.txt", gamedir);

	error_t err = cpymo_gameconfig_parse_from_file(&gameconfig, buf);
	if (err != CPYMO_ERR_SUCC) {
		printf("Error: Cannot parse gameconfig.txt. (%s)", cpymo_error_message(err));
		exit(-1);
	}
}

int main(int argc, char **argv) 
{
	const char *gamedir = "./";

	if (argc == 2) {
		gamedir = argv[1];
	}

	load_gameconfig(gamedir);
	error_t err = 
		cpymo_assetloader_init(&assetloader, &gameconfig, gamedir);

	if (err != CPYMO_ERR_SUCC) {
		SDL_Log("Error: cpymo_assetloader_init (%s)", cpymo_error_message(err));
	}

	if (SDL_Init(
		SDL_INIT_EVENTS |
		SDL_INIT_AUDIO |
		SDL_INIT_GAMECONTROLLER |
		SDL_INIT_JOYSTICK |
		SDL_INIT_VIDEO) != 0) {
		SDL_Log("Error: Unable to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

	if (SDL_CreateWindowAndRenderer(
		gameconfig.imagesize_w,
		gameconfig.imagesize_h,
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE,
		&window,
		&renderer) != 0) {
		SDL_Log("Error: Can not create window and renderer: %s", SDL_GetError());
		return -1;
	}

	SDL_SetWindowTitle(window, gameconfig.gametitle);
	set_window_icon(gamedir);
	
	if (SDL_RenderSetLogicalSize(renderer, gameconfig.imagesize_w, gameconfig.imagesize_h) != 0) {
		SDL_Log("Error: Can not set logical size: %s", SDL_GetError());
		return -1;
	}

	error_t error = cpymo_engine_init(&engine, &gameconfig);
	if (error != CPYMO_ERR_SUCC) {
		SDL_Log("Error: Can not init cpymo engine, %s", cpymo_error_message(error));
	}
	
	SDL_Event event;
	while (1) {
		bool redraw_by_event = false;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				goto EXIT;
			else if (event.type == SDL_WINDOWEVENT)
				redraw_by_event = true;
		}

		bool need_to_redraw = false;

		cpymo_engine_update(&engine, 16, &need_to_redraw);

		if (need_to_redraw || redraw_by_event) {
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
			SDL_RenderClear(renderer);
			cpymo_engine_draw(&engine);
			SDL_RenderPresent(renderer);
		} else SDL_Delay(50);
	}

	EXIT:

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	cpymo_assetloader_free(&assetloader);

	return 0;
}
