#include <stdlib.h>
#include <stdio.h>
#include <cpymo_error.h>
#include "cpymo_import_sdl2.h"
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
#elif defined __PSP__
#include <pspkernel.h>
#include <pspdebug.h>

/* Define the module info section */
/*PSP_MODULE_INFO("CPyMO", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_MAX();*/
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

extern error_t cpymo_backend_font_init(const char *gamedir);
extern void cpymo_backend_font_free();

extern void cpymo_backend_audio_init();
extern void cpymo_backend_audio_free();

static void set_window_icon(const char *gamedir) 
{
#if (defined _WIN32) || (defined __LINUX__) || (defined __APPLE__)
	int w, h;
	char *icon_path = (char *)alloca(strlen(gamedir) + 10);
	if (icon_path == NULL) return;

	sprintf(icon_path, "%s/icon.png", gamedir);

	stbi_uc *icon = stbi_load(icon_path, &w, &h, NULL, 4);
	if (icon == NULL) return;

	if (w != 57 || h != 57) {
		printf("[Warning] Icon must be 57x57.\n");
	}

	Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	SDL_Surface *surface =
		SDL_CreateRGBSurfaceFrom(icon, w, h, 4 * 8, 4 * w, rmask, gmask, bmask, amask);

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

cpymo_game_selector_item *get_game_list(const char *game_selector_dir);

static error_t after_start_game(cpymo_engine *e, const char *gamedir)
{
	save_last_selected_game_dir(gamedir);

	if (SDL_RenderSetLogicalSize(renderer, e->gameconfig.imagesize_w, e->gameconfig.imagesize_h) != 0) {
		return CPYMO_ERR_UNKNOWN;
	}

#ifndef __PSP__
	cpymo_backend_font_free();
	error_t err = cpymo_backend_font_init(gamedir);
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
cpymo_game_selector_item *get_game_list(const char *game_selector_dir)
{
	cpymo_game_selector_item *item = NULL;
	cpymo_game_selector_item *item_tail = NULL;

	DIR *dir = opendir(game_selector_dir);

	if (dir) {
		struct dirent* ent;
		while ((ent = readdir(dir))) {
			char *path = (char *)malloc(strlen(ent->d_name) + strlen(game_selector_dir) + 4);
			sprintf(path, "%s/%s", game_selector_dir, ent->d_name);

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

static error_t create_window_and_renderer(int width, int height, SDL_Window **window, SDL_Renderer **renderer)
{
#ifdef USE_GAME_SELECTOR
	const char *title = "CPyMO";
#else
	const char *title = engine.gameconfig.gametitle;
#endif

	*window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
		SDL_WINDOW_ALLOW_HIGHDPI
		| SDL_WINDOW_RESIZABLE
#ifdef __ANDROID__
		| SDL_WINDOW_FULLSCREEN
#endif
	);

	if (*window == NULL) {
		SDL_Log("[Error] Can not create window: %s.", SDL_GetError());
		return CPYMO_ERR_UNKNOWN;
	}


#ifdef FRAMEBUFFER_PIXELFORMAT
	SDL_DisplayMode display_mode;
	memset(&display_mode, 0, sizeof(display_mode));
	
	display_mode.format = FRAMEBUFFER_PIXELFORMAT;
	display_mode.w = SCREEN_WIDTH;
	display_mode.h = SCREEN_HEIGHT;
	
	if (SDL_SetWindowDisplayMode(*window, &display_mode) != 0) {
		SDL_Log("[Warning] Can not set window display mode.");
	}
#endif


	*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_PRESENTVSYNC);
	if (*renderer == NULL) {
		SDL_DestroyWindow(*window);
		*window = NULL;
		SDL_Log("[Error] Can not create renderer: %s.", SDL_GetError());
		return CPYMO_ERR_UNKNOWN;
	}

	return CPYMO_ERR_SUCC;
}


int main(int argc, char **argv)
{
	srand((unsigned)time(NULL));
	//_CrtSetBreakAlloc(1371);

#if (!(defined DISABLE_FFMPEG_AUDIO) && !(defined DISABLE_FFMPEG_MOVIE))
	av_log_set_level(AV_LOG_ERROR);
#endif

#ifdef __ANDROID__
	SDL_AndroidRequestPermission("android.permission.MANAGE_EXTERNAL_STORAGE");
	SDL_AndroidRequestPermission("android.permission.WRITE_EXTERNAL_STORAGE");
	SDL_AndroidRequestPermission("android.permission.READ_EXTERNAL_STORAGE");
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
	cpymo_game_selector_item *item = get_game_list(GAME_SELECTOR_DIR);

#ifdef GAME_SELECTOR_DIR_2
	{
		cpymo_game_selector_item *item2 = get_game_list(GAME_SELECTOR_DIR_2);
		if (item == NULL) item = item2;
		else {
			while (item->next != NULL) item = item->next;
			item->next = item2;
		}
	}
#endif
	
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
	
	if (create_window_and_renderer(window_size_w, window_size_h, &window, &renderer) != CPYMO_ERR_SUCC) {
		cpymo_engine_free(&engine);
		cpymo_backend_audio_free();
		SDL_Log("[Error] Can not create window and renderer: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

#ifndef USE_GAME_SELECTOR
	set_window_icon(gamedir);
	ensure_save_dir(gamedir);
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
	if (cpymo_backend_font_init(gamedir) != CPYMO_ERR_SUCC) {
#else
	if (cpymo_backend_font_init(NULL) != CPYMO_ERR_SUCC) {
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

	size_t redraw_by_event = 30;
	while (
		1
#ifdef __SWITCH
		&& appletMainLoop()
#endif
		) {

		extern float mouse_wheel;
		mouse_wheel = 0;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_WINDOWEVENT || event.type == SDL_RENDER_TARGETS_RESET)
				redraw_by_event++;
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
			else if (event.type == SDL_APP_WILLENTERFOREGROUND || event.type == SDL_APP_DIDENTERFOREGROUND) {
				redraw_by_event++;
			}
			else if (event.type == SDL_APP_TERMINATING || event.type == SDL_APP_LOWMEMORY)
				goto EXIT;
#ifndef __ANDROID__
			else if (event.type == SDL_QUIT)
                goto EXIT;
#endif
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
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CPyMO Error", cpymo_error_message(err), window);
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
			if (redraw_by_event) redraw_by_event--;
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
