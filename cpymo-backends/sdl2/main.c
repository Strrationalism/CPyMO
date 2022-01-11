#include <stdlib.h>
#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>
#include <cpymo_engine.h>
#include <cpymo_interpreter.h>
#include <string.h>
#include <cpymo_backend_text.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if _WIN32 && !NDEBUG
#include <crtdbg.h>
#endif

SDL_Window *window;
SDL_Renderer *renderer;
cpymo_engine engine;

extern error_t cpymo_backend_font_init(const char *gamedir);
extern void cpymo_backend_font_free();

static void set_window_icon(const char *gamedir) 
{
	int w, h, channel;
	char *icon_path = (char *)malloc(strlen(gamedir) + 10);
	if (icon_path == NULL) return;

	sprintf(icon_path, "%s/icon.png", gamedir);

	stbi_uc *icon = stbi_load(icon_path, &w, &h, &channel, 4);
	free(icon_path);
	if (icon == NULL) return;

	SDL_Surface *surface =
		SDL_CreateRGBSurfaceFrom(icon, w, h, channel * 8, channel * w, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

	if (surface == NULL) {
		stbi_image_free(icon);
		return;
	}

	SDL_SetWindowIcon(window, surface);
	SDL_FreeSurface(surface);
	stbi_image_free(icon);
}

int main(int argc, char **argv) 
{
	const char *gamedir = "./";

	if (argc == 2) {
		gamedir = argv[1];
	}

	error_t err = cpymo_engine_init(&engine, gamedir);

	if (err != CPYMO_ERR_SUCC) {
		SDL_Log("[Error] cpymo_engine_init (%s)", cpymo_error_message(err));
		return -1;
	}

	if (SDL_Init(
		SDL_INIT_EVENTS |
		SDL_INIT_AUDIO |
		SDL_INIT_VIDEO) != 0) {
		cpymo_engine_free(&engine);
		SDL_Log("[Error] Unable to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

#if !NDEBUG
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
#endif

	if (SDL_CreateWindowAndRenderer(
		engine.gameconfig.imagesize_w,
		engine.gameconfig.imagesize_h,
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE,
		&window,
		&renderer) != 0) {
		cpymo_engine_free(&engine);
		SDL_Log("[Error] Can not create window and renderer: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_SetWindowTitle(window, engine.gameconfig.gametitle);
	set_window_icon(gamedir);
	
	if (SDL_RenderSetLogicalSize(renderer, engine.gameconfig.imagesize_w, engine.gameconfig.imagesize_h) != 0) {
		SDL_Log("[Error] Can not set logical size: %s", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		cpymo_engine_free(&engine);
		SDL_Quit();
		return -1;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	if (cpymo_backend_font_init(gamedir) != CPYMO_ERR_SUCC) {
		SDL_Log("[Error] Can not find font file, try put default.ttf into <gamedir>/system/.");
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		cpymo_engine_free(&engine);
		SDL_Quit();
		return -1;
	}

	cpymo_backend_text text;
	cpymo_backend_text_create(&text, "CPyMO for 3DS", 32.0f);
	
	Uint64 prev_ticks = SDL_GetTicks64();
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

		Uint64 ticks = SDL_GetTicks64();
		err = cpymo_engine_update(
			&engine, 
			(float)(ticks - prev_ticks) * 0.001f, 
			&need_to_redraw);

		switch (err) {
		case CPYMO_ERR_SUCC: break;
		case CPYMO_ERR_NO_MORE_CONTENT: goto EXIT;
		default: {
			SDL_Log("[Error] %s.", cpymo_error_message(err));
			goto EXIT;
		}
		}

		prev_ticks = ticks;

		if (need_to_redraw || redraw_by_event) {
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
			SDL_RenderClear(renderer);
			cpymo_engine_draw(&engine);
			cpymo_color col;
			col.r = 255;
			col.g = 255;
			col.b = 255;
			cpymo_backend_text_draw(text, 0, 32, col, 1.0f, cpymo_backend_image_draw_type_text_ui);
			SDL_RenderPresent(renderer);
		} else SDL_Delay(50);
	}

	EXIT:
	cpymo_backend_text_free(text);

	cpymo_backend_font_free();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	cpymo_engine_free(&engine);

	#if _WIN32 && !NDEBUG
	_CrtDumpMemoryLeaks();
	#endif

	return 0;
}
