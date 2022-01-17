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

void print_banner() {
	puts("      ...              ....      ..                      ...     ..      ..             ....");
	puts("   xH88\"`~ .x8X      +^\"\"888h. ~\"888h     ..           x*8888x.:*8888: -\"888:       .x~X88888Hx.");
	puts(" :8888   .f\"8888Hf  8X.  ?8888X  8888f   @L           X   48888X `8888H  8888      H8X 888888888h.");
	puts(":8888>  X8L  ^\"\"`  \'888x  8888X  8888~  9888i   .dL  X8x.  8888X  8888X  !888>    8888:`*888888888:");
	puts("X8888  X888h       \'88888 8888X   \"88x: `Y888k:*888. X8888 X8888  88888   \"*8%-   88888:        `%8");
	puts("88888  !88888.      `8888 8888X  X88x.    888E  888I \'*888!X8888> X8888  xH8>   . `88888          ?>");
	puts("88888   %88888        `*` 8888X \'88888X   888E  888I   `?8 `8888  X888X X888>   `. ?888%           X");
	puts("88888 \'> `8888>      ~`...8888X  \"88888   888E  888I   -^  \'888\"  X888  8888>     ~*??.            >");
	puts("`8888L %  ?888   !    x8888888X.   `%8\"   888E  888I    dx \'88~x. !88~  8888>    .x88888h.        <");
	puts(" `8888  `-*\"\"   /    \'%\"*8888888h.   \"   x888N><888\'  .8888Xf.888x:!    X888X.: :\"\"\"8888888x..  .x");
	puts("   \"888.      :\"     ~    888888888!`     \"88\"  888  :\"\"888\":~\"888\"     `888*\"  `    `*888888888\"");
	puts("     `\"\"***~\"`            X888^\"\"\"              88F      \"~\'    \"~        \"\"            \"\"***\"\"");
	puts("                          `88f                 98\"");
	puts("                           88                ./\"");
	puts("                           \"\"               ~`");
	puts("");
}

int main(int argc, char **argv) 
{
	print_banner();

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
			SDL_RenderPresent(renderer);
		} else SDL_Delay(50);
	}

	EXIT:
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
