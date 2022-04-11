#include <stdlib.h>
#include <stdio.h>
#include <cpymo_error.h>
#include <SDL.h>
#include <cpymo_engine.h>
#include <cpymo_interpreter.h>
#include <string.h>
#include <cpymo_backend_text.h>

#if (!(defined DISABLE_FFMPEG_AUDIO) && !(defined DISABLE_FFMPEG_MOVIE))
#include <libavutil/log.h>
#endif

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

#if defined __SWITCH__
#include <switch.h>
#elif defined __EMSCRIPTEN__
#include <emscripten.h> 
#define SDL_Delay emscripten_sleep
#endif

#if _WIN32 && !NDEBUG
#include <crtdbg.h>
#endif

#ifdef __APPLE__
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if (defined _WIN32) || (defined __LINUX__) || (defined __APPLE__)
#define ENABLE_ALT_ENTER_FULLSCREEN
#endif

#include "posix_win32.h"

SDL_Window *window;
SDL_Renderer *renderer;
cpymo_engine engine;

extern error_t cpymo_backend_font_init(const char *gamedir, const char *fallback);
extern void cpymo_backend_font_free();

extern void cpymo_backend_audio_init();
extern void cpymo_backend_audio_free();

static void set_window_icon(const char *gamedir) 
{
#if (defined _WIN32) || (defined __LINUX__) || (defined __APPLE__)
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

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
const char *get_emscripten_game_dir(void) {
	return EMSCRIPTEN_GAMEDIR;
}

#include <cpymo_save_global.h>

EMSCRIPTEN_KEEPALIVE
void reload_global_save(void) {
	cpymo_save_config_load(&engine);
	cpymo_save_global_load(&engine);
}

#endif

static void ensure_save_dir(const char *gamedir)
{
	char *save_dir = (char *)alloca(strlen(gamedir) + 8);
	sprintf(save_dir, "%s/save", gamedir);
	mkdir(save_dir, 0777);
#ifdef __EMSCRIPTEN__
	EM_ASM(
		var gamedir = Module.ccall('get_emscripten_game_dir', 'string');
		FS.mount(IDBFS, {}, gamedir + '/save');

		FS.syncfs(true, function(err) {
			Module.ccall('reload_global_save');
		});
	);
#endif
}

#ifdef USE_GAME_SELECTOR
static char *get_last_selected_game_dir()
{
#if (defined __SWITCH__ || defined __PSP__ || defined __PSV__)
	char *str = NULL;
	size_t len;
	error_t err = cpymo_utils_loadfile(GAME_SELECTOR_DIR "/last_game.txt", &str, &len);

	if (err != CPYMO_ERR_SUCC) return NULL;

	char *ret = realloc(str, len + 1);
	if (ret == NULL) {
		free(str);
		return NULL;
	}

	ret[len] = '\0';
	return ret;

#else
	return NULL;
#endif
}

static void save_last_selected_game_dir(const char *gamedir)
{
#if (defined __SWITCH__ || defined __PSP__ || defined __PSV__)
	size_t len = strlen(gamedir);
	FILE *f = fopen(GAME_SELECTOR_DIR "/last_game.txt", "wb");
	if (f == NULL) return;

	fwrite(gamedir, len, 1, f);
	fclose(f);
#endif
}
#endif

#ifdef USE_GAME_SELECTOR
#include <cpymo_game_selector.h>
static error_t after_start_game(cpymo_engine *e, const char *gamedir)
{
	save_last_selected_game_dir(gamedir);

	if (SDL_RenderSetLogicalSize(renderer, e->gameconfig.imagesize_w, e->gameconfig.imagesize_h) != 0) {
		return CPYMO_ERR_UNKNOWN;
	}

#ifndef __PSP__
	cpymo_backend_font_free();
	error_t err = cpymo_backend_font_init(gamedir, GAME_SELECTOR_DIR "/default.ttf");
	CPYMO_THROW(err);
#endif

	ensure_save_dir(gamedir);
	set_window_icon(gamedir);

	SDL_SetWindowTitle(window, engine.gameconfig.gametitle);

	return CPYMO_ERR_SUCC;
}
#endif


#if ((defined __SWITCH__ || defined __PSP__ || defined __PSV__ || defined __ANDROID__) && defined USE_GAME_SELECTOR)

#include <dirent.h>
cpymo_game_selector_item *get_game_list(void)
{
	cpymo_game_selector_item *item = NULL;
	cpymo_game_selector_item *item_tail = NULL;

	DIR *dir = opendir(GAME_SELECTOR_DIR);

	if (dir) {
		struct dirent* ent;
		while ((ent = readdir(dir))) {
			char *path = (char *)malloc(strlen(ent->d_name) + strlen(GAME_SELECTOR_DIR) + 4);
			sprintf(path, GAME_SELECTOR_DIR "/%s", ent->d_name);

			cpymo_game_selector_item *cur = NULL;
			error_t err = cpymo_game_selector_item_create(&cur, &path);
			if (err != CPYMO_ERR_SUCC) {
				free(path);
				continue;
			}

			if (item == NULL) {
				item = cur;
				item_tail = cur;
			}
			else {
				item_tail->next = cur;
				item_tail = cur;
			}

		}
		closedir(dir);
	}

	return item;
}
#endif

int main(int argc, char **argv)
{
	//_CrtSetBreakAlloc(1371);

#if (!(defined DISABLE_FFMPEG_AUDIO) && !(defined DISABLE_FFMPEG_MOVIE))
	av_log_set_level(AV_LOG_ERROR);
#endif

#ifndef DISABLE_FFMPEG_AUDIO
	engine.audio.enabled = false;
#endif

	int ret = 0;

#ifndef USE_GAME_SELECTOR
	const char *gamedir = "./";

#ifndef __EMSCRIPTEN__
	if (argc == 2) {
		gamedir = argv[1];
	}
#else
	gamedir = EMSCRIPTEN_GAMEDIR;
#endif

	ensure_save_dir(gamedir);
#endif

	// SDL2 has 2 memory leaks!
	if (SDL_Init(
		SDL_INIT_EVENTS |
#ifndef DISABLE_AUDIO
		SDL_INIT_AUDIO |
#endif
		SDL_INIT_VIDEO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMECONTROLLER) != 0) {
		SDL_Log("[Error] Unable to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	cpymo_backend_audio_init();

#ifndef USE_GAME_SELECTOR
	error_t err = cpymo_engine_init(&engine, gamedir);
#else
	char *last_selected_game_dir = get_last_selected_game_dir();
	cpymo_game_selector_item *item = get_game_list();
	error_t err = cpymo_engine_init_with_game_selector(
		&engine, SCREEN_WIDTH, SCREEN_HEIGHT,
		GAME_SELECTOR_FONTSIZE,
		GAME_SELECTOR_EMPTY_MSG_FONTSIZE,
		GAME_SELECTOR_COUNT_PER_SCREEN,
		&item, NULL, &after_start_game,
		&last_selected_game_dir);

	if (err != CPYMO_ERR_SUCC) {
		if (last_selected_game_dir) free(last_selected_game_dir);
		cpymo_game_selector_item_free_all(item);
	}
#endif

	if (err != CPYMO_ERR_SUCC) {
		cpymo_backend_audio_free();
		SDL_Log("[Error] cpymo_engine_init (%s)", cpymo_error_message(err));
		SDL_Quit();
		return -1;
	}

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");

#if (defined SCREEN_WIDTH && defined SCREEN_HEIGHT)
#ifdef ADAPT_SCREEN_SIZE
	uint16_t window_size_w, window_size_h;
	{
		SDL_Rect r;
		if (SDL_GetDisplayBounds(0, &r) == 0) {
			window_size_w = (uint16_t)r.w;
			window_size_h = (uint16_t)r.h;
		}
		else {
			window_size_w = SCREEN_WIDTH;
			window_size_h = SCREEN_HEIGHT;
		}
	}
#else
	const uint16_t window_size_w = SCREEN_WIDTH;
	const uint16_t window_size_h = SCREEN_HEIGHT;
#endif
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
		cpymo_backend_audio_free();
		SDL_Log("[Error] Can not create window and renderer: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

#ifndef USE_GAME_SELECTOR
	set_window_icon(gamedir);
	SDL_SetWindowTitle(window, engine.gameconfig.gametitle);
#else
	SDL_SetWindowTitle(window, "CPyMO");
#endif

	if (SDL_RenderSetLogicalSize(renderer, engine.gameconfig.imagesize_w, engine.gameconfig.imagesize_h) != 0) {
		SDL_Log("[Error] Can not set logical size: %s", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		cpymo_engine_free(&engine);
		cpymo_backend_audio_free();
		SDL_Quit();
		return -1;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

#ifndef USE_GAME_SELECTOR
	if (cpymo_backend_font_init(gamedir, NULL) != CPYMO_ERR_SUCC) {
#else
	if (cpymo_backend_font_init(NULL, GAME_SELECTOR_DIR "/default.ttf") != CPYMO_ERR_SUCC) {
#endif
		SDL_Log("[Error] Can not find font file, try put default.ttf into <gamedir>/system/.");
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		cpymo_engine_free(&engine);
		cpymo_backend_audio_free();
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
#ifdef ENABLE_ALT_ENTER_FULLSCREEN
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT)) {
					if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
						SDL_SetWindowFullscreen(window, 0);
					}
					else {
						SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
					}
				}
			}
#endif
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
