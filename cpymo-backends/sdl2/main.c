#include <stdlib.h>
#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>
#include <cpymo_engine.h>
#include <cpymo_interpreter.h>
#include <string.h>
#include <cpymo_backend_text.h>
#include <libavutil/log.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define FASTEST_FILTER STBIR_FILTER_BOX
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE  FASTEST_FILTER
#define STBIR_DEFAULT_FILTER_UPSAMPLE    FASTEST_FILTER
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>
#include <cpymo_backend_audio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifdef __SWITCH__
#include <switch.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define GAME_SELECTOR_DIR "/pymogames/"
#endif


#if _WIN32 && !NDEBUG
#include <crtdbg.h>
#endif

#ifdef __APPLE__
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "posix_win32.h"

SDL_Window *window;
SDL_Renderer *renderer;
cpymo_engine engine;

extern error_t cpymo_backend_font_init(const char *gamedir);
extern void cpymo_backend_font_free();

extern void cpymo_backend_audio_init();
extern void cpymo_backend_audio_free();


static void set_window_icon(const char *gamedir) 
{
#ifndef __SWITCH__
	int w, h, channel;
	char *icon_path = (char *)alloca(strlen(gamedir) + 10);
	if (icon_path == NULL) return;

	sprintf(icon_path, "%s/icon.png", gamedir);

	stbi_uc *icon = stbi_load(icon_path, &w, &h, &channel, 4);
	//free(icon_path);
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
#endif
}

static void ensure_save_dir(const char *gamedir)
{
	char *save_dir = (char *)alloca(strlen(gamedir) + 8);
	sprintf(save_dir, "%s/save", gamedir);
	mkdir(save_dir, 0777);
}

int main(int argc, char **argv) 
{
	//_CrtSetBreakAlloc(1371);

	av_log_set_level(AV_LOG_ERROR);

	engine.audio.enabled = false;

	int ret = 0;

#ifdef __SWITCH__
	const char *gamedir = "./pymogames/startup";
#else
	const char *gamedir = "./";
#endif

	if (argc == 2) {
		gamedir = argv[1];
	}

	ensure_save_dir(gamedir);

	// SDL2 has 2 memory leaks!
	if (SDL_Init(
		SDL_INIT_EVENTS |
		SDL_INIT_AUDIO |
		SDL_INIT_VIDEO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMECONTROLLER) != 0) {
		SDL_Log("[Error] Unable to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	cpymo_backend_audio_init();

	error_t err = cpymo_engine_init(&engine, gamedir);

	if (err != CPYMO_ERR_SUCC) {
		SDL_Log("[Error] cpymo_engine_init (%s)", cpymo_error_message(err));
		SDL_Quit();
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

#if (defined SCREEN_WIDTH && defined SCREEN_HEIGHT)
	const uint16_t window_size_w = SCREEN_WIDTH;
	const uint16_t window_size_h = SCREEN_HEIGHT;
#else
	const uint16_t window_size_w = engine.gameconfig.imagesize_w;
	const uint16_t window_size_h = engine.gameconfig.imagesize_h;
#endif

	if (SDL_CreateWindowAndRenderer(
		window_size_w,
		window_size_h,
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

	Uint32 prev_ticks = SDL_GetTicks();
	SDL_Event event;

	// unsigned fps_counter = 0;
	// Uint32 fps_timer = 0;

	if (cpymo_backend_audio_get_info()) {
		SDL_UnlockAudio();
	}

	while (
		1
#ifdef __SWITCH
		&& appletMainLoop()
#endif
		) {
		bool redraw_by_event = false;

		extern float mouse_wheel;
		mouse_wheel = 0;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				goto EXIT;
			else if (event.type == SDL_WINDOWEVENT || event.type == SDL_RENDER_TARGETS_RESET)
				redraw_by_event = true;
			else if (event.type == SDL_MOUSEWHEEL) {
				mouse_wheel = (float)event.wheel.y;
				if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
					mouse_wheel *= -1;
			}
			else if (event.type == SDL_AUDIODEVICEADDED || event.type == SDL_AUDIODEVICEREMOVED) {
				extern void cpymo_backend_audio_reset();
				cpymo_backend_audio_reset();
			}
			else if (event.type == SDL_RENDER_DEVICE_RESET) {
				SDL_Log("[Error] Render device lost!\n");
				ret = -1;
				goto EXIT;
			}
			else if (event.type == SDL_JOYDEVICEADDED || event.type == SDL_JOYDEVICEREMOVED ||
				event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED ||
				event.type == SDL_CONTROLLERDEVICEREMAPPED) {
				extern void cpymo_input_refresh_joysticks();
				cpymo_input_refresh_joysticks();
			}
		}

		bool need_to_redraw = false;

		Uint32 ticks = SDL_GetTicks();
		err = cpymo_engine_update(
			&engine, 
			(float)(ticks - prev_ticks) * 0.001f, 
			&need_to_redraw);

		switch (err) {
		case CPYMO_ERR_SUCC: break;
		case CPYMO_ERR_NO_MORE_CONTENT: goto EXIT;
		default: {
			SDL_Log("[Error] %s.", cpymo_error_message(err));
			ret = -1;
			goto EXIT;
		}
		}

		/*fps_timer += (ticks - prev_ticks);
		if (fps_timer >= 1000) {
			//printf("[FPS] %u\n", fps_counter);
			fps_timer -= 1000;
			fps_counter = 0;
		}*/
		prev_ticks = ticks;

		if (need_to_redraw || redraw_by_event) {
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
			SDL_RenderClear(renderer);
			cpymo_engine_draw(&engine);
			SDL_RenderPresent(renderer);
			//fps_counter++;
		} else SDL_Delay(16);
	}

	EXIT:
	cpymo_engine_free(&engine);

	extern void cpymo_input_free_joysticks();
	cpymo_input_free_joysticks();

	cpymo_backend_font_free();
	cpymo_backend_audio_free();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();


	#if _WIN32 && !NDEBUG
	_CrtDumpMemoryLeaks();
	#endif

	return ret;
}
