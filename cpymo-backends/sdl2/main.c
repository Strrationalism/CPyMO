#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>
#include <cpymo_gameconfig.h>
#include <cpymo_parser.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

cpymo_gameconfig gameconfig;
SDL_Window *window;
SDL_Renderer *renderer;

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
	}

	SDL_SetWindowTitle(window, gameconfig.gametitle);

	set_window_icon(gamedir);
	

	SDL_Event event;
	while (1) {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT)
			break;


		// TODO: This is for temp using
		// Need to design a well screen update method.
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}
