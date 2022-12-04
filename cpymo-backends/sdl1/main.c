#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#define FASTEST_FILTER STBIR_FILTER_BOX
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE  FASTEST_FILTER
#define STBIR_DEFAULT_FILTER_UPSAMPLE    FASTEST_FILTER

#ifdef LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#endif

#include <cpymo_prelude.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#include <SDL/SDL.h>
#include <cpymo_engine.h>
#include <cpymo_error.h>
#include <time.h>

cpymo_engine engine;
SDL_Surface *framebuffer;
extern int mouse_wheel;

#ifndef SCREEN_BPP
#define SCREEN_BPP 24
#endif

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH engine.gameconfig.imagesize_w
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT engine.gameconfig.imagesize_h
#endif

#ifndef SCREEN_FLAGS
#define SCREEN_FLAGS (SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_ANYFORMAT | SDL_HWPALETTE | SDL_DOUBLEBUF)
#endif

extern error_t cpymo_backend_font_init(const char *gamedir);
extern void cpymo_backend_font_free();

static bool current_full_screen;

static void set_clip_rect(size_t screen_w, size_t screen_h) 
{
    SDL_Rect clip;
    clip.x = (screen_w - engine.gameconfig.imagesize_w) / 2;
    clip.y = (screen_h - engine.gameconfig.imagesize_h) / 2;
    clip.w = engine.gameconfig.imagesize_w;
    clip.h = engine.gameconfig.imagesize_h;

    SDL_SetClipRect(framebuffer, &clip);
}

static error_t set_video_mode(size_t w, size_t h, bool fullscreen) 
{
    Uint32 flags = SCREEN_FLAGS;
    if (fullscreen)
        flags |= SDL_FULLSCREEN;

#ifdef SCREEN_RESIZABLE
    flags |= SDL_RESIZABLE;
#endif

    current_full_screen = fullscreen;

    framebuffer = SDL_SetVideoMode((int)w, (int)h, SCREEN_BPP, flags);
    if (framebuffer == NULL)
        return CPYMO_ERR_UNKNOWN;
    
    set_clip_rect(w, h);
    return CPYMO_ERR_SUCC;
}

#ifndef LOAD_GAME_ICON
#define load_game_icon(...)
#else 
static inline void load_game_icon(const char *gamedir) 
{
    char *path = alloca(strlen(gamedir) + 10);
    sprintf(path, "%s/icon.png", gamedir);

    int w, h;
    stbi_uc *data = stbi_load(path, &w, &h, NULL, 4);
    if (data == NULL) return;

    if (w != 57 || h != 57) {
        printf("[Warning] Icon must be 57x57.\n");
    }

    void *px = (void *)malloc(32 * 32 * 4);
    if (px == NULL) {
        free(data);
        return;
    }

    stbir_resize_uint8(data, w, h, 0, px, 32, 32, 0, 4);
    free(data);

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

    SDL_Surface *sur = SDL_CreateRGBSurfaceFrom(
		px,
		32,
		32,
		32,
		4 * 32,
		rmask,
		gmask,
		bmask,
		amask);

    if (sur == NULL) {
        free(px);
        return;
    }

    SDL_WM_SetIcon(sur, NULL);
    SDL_FreeSurface(sur);
    free(px);
}
#endif

#ifdef _WIN32
#include <direct.h>
#define mkdir(x, y) _mkdir(x)
#else
#include <sys/stat.h>
#endif

static void ensure_save_dir(const char *gamedir)
{
	char *save_dir = (char *)alloca(strlen(gamedir) + 8);
	sprintf(save_dir, "%s/save", gamedir);
	mkdir(save_dir, 0777);
}

#ifdef USE_GAME_SELECTOR
#include <cpymo_game_selector.h>

#include <dirent.h>

static char *get_last_selected_game_dir()
{
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
}

static void save_last_selected_game_dir(const char *gamedir)
{
	size_t len = strlen(gamedir);
	FILE *f = fopen(GAME_SELECTOR_DIR "/last_game.txt", "wb");
	if (f == NULL) return;

	fwrite(gamedir, len, 1, f);
	fclose(f);
}

static error_t after_start_game(cpymo_engine *e, const char *gamedir)
{
	save_last_selected_game_dir(gamedir);

#ifdef GAME_SELECTOR_RESET_SCREEN_SIZE_AFTER_START_GAME
	set_video_mode(e->gameconfig.imagesize_w, e->gameconfig.imagesize_h, current_full_screen);
#else
    SDL_FillRect(framebuffer, NULL, 0);
    set_clip_rect(SCREEN_WIDTH, SCREEN_HEIGHT);
#endif

#if !(defined __PSP__ || defined __WII__)
	cpymo_backend_font_free();
	error_t err = cpymo_backend_font_init(gamedir);
	CPYMO_THROW(err);
#endif

	ensure_save_dir(gamedir);

    SDL_WM_SetCaption(
        engine.gameconfig.gametitle,
        engine.gameconfig.gametitle);

	return CPYMO_ERR_SUCC;
}

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

#if (!(defined DISABLE_FFMPEG_AUDIO) && !(defined DISABLE_FFMPEG_MOVIE))
#include <libavutil/log.h>
#endif

#ifdef __WII__
#include <fat.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

#ifdef ENABLE_EXIT_CONFIRM
#include <cpymo_msgbox_ui.h>
#include <cpymo_localization.h>
static error_t cpymo_exit_confirm(struct cpymo_engine *e, void *data)
{
	return CPYMO_ERR_NO_MORE_CONTENT;
}
#endif

#ifdef __PSP__
#define main SDL_main
#include <psppower.h>
#endif

int main(int argc, char **argv) 
{
#ifdef __PSP__    
    scePowerSetCpuClockFrequency(333);
    scePowerSetBusClockFrequency(167);
#endif

    srand((unsigned)time(NULL));

#if (!(defined DISABLE_FFMPEG_AUDIO) && !(defined DISABLE_FFMPEG_MOVIE))
	av_log_set_level(AV_LOG_ERROR);
#endif

#ifdef __WII__
    fatInitDefault();
    WPAD_Init();
    stbi_set_flip_vertically_on_load_thread(false);
#endif

    if (SDL_Init(
            SDL_INIT_VIDEO
#ifndef DISABLE_AUDIO
            | SDL_INIT_AUDIO
#endif
#ifdef __PSP__
            | SDL_INIT_JOYSTICK
#endif
        ) < 0) {
        printf("[Error] SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

#if defined __WII__ || defined __PSP__
    SDL_ShowCursor(SDL_DISABLE);
#endif

    extern void cpymo_backend_audio_init(void);
    extern void cpymo_backend_audio_free(void);

    cpymo_backend_audio_init();

    extern error_t cpymo_backend_image_init(void);
    extern void cpymo_backend_image_quit(void);
    error_t err = cpymo_backend_image_init();
    if (err != CPYMO_ERR_SUCC) {
        cpymo_backend_audio_free();
        SDL_Quit();
        printf("[Error] cpymo_backend_image_init: %s\n", 
            cpymo_error_message(err));
        return err;
    }

#ifdef USE_GAME_SELECTOR
    cpymo_game_selector_item *items = get_game_list(GAME_SELECTOR_DIR);
    char *last_selected = get_last_selected_game_dir();
    err = cpymo_engine_init_with_game_selector(
        &engine, 
        SCREEN_WIDTH, SCREEN_HEIGHT,
        GAME_SELECTOR_FONTSIZE,
        GAME_SELECTOR_EMPTY_MSG_FONTSIZE,
        GAME_SELECTOR_COUNT_PER_SCREEN,
        &items,
        NULL,
        &after_start_game,
        &last_selected);

    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_engine_init_with_game_selector: %s\n", cpymo_error_message(err));
        cpymo_game_selector_item_free_all(items);
        free(last_selected);
        cpymo_backend_image_quit();
        cpymo_backend_audio_free();
        SDL_Quit();
        return -1;
    }

    err = cpymo_backend_font_init(NULL);
#else    
    const char *gamedir = ".";

    if (argc > 1) {
        gamedir = argv[1];
    }

    load_game_icon(gamedir);

    err = cpymo_engine_init(&engine, gamedir);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_engine_init: %s\n", cpymo_error_message(err));
        cpymo_backend_image_quit();
        cpymo_backend_audio_free();
        SDL_Quit();
        return -1;
    }

    ensure_save_dir(gamedir);

    err = cpymo_backend_font_init(gamedir);
#endif
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] cpymo_backend_font_init: %s\n", cpymo_error_message(err));
        cpymo_engine_free(&engine);
        cpymo_backend_image_quit();
        cpymo_backend_audio_free();
        SDL_Quit();
        return -1;
    }
    
    err = set_video_mode(SCREEN_WIDTH, SCREEN_HEIGHT, 
#ifdef DEFAULT_FULLSCREEN
    true
#else
    false
#endif
    );

    if (framebuffer == NULL) {
        printf("[Error] SDL_SetVideoMode: %s\n", SDL_GetError());
        cpymo_engine_free(&engine);
        cpymo_backend_image_quit();
        cpymo_backend_audio_free();
        cpymo_backend_font_free();
        SDL_Quit();
        return -1;
    }

    SDL_WM_SetCaption(
        engine.gameconfig.gametitle,
        engine.gameconfig.gametitle);

    Uint32 prev_time = SDL_GetTicks();
    int ret = 0;

    while (1) {
        bool redraw_system = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                #ifdef ENABLE_EXIT_CONFIRM
                #ifndef DISABLE_MOVIE
                extern bool playing_movie;
                if (playing_movie) goto EXIT;
                #endif
				
				if (cpymo_ui_enabled(&engine))
                	cpymo_ui_exit(&engine);

				err = cpymo_msgbox_ui_enter(
					&engine,
					cpymo_str_pure(
						cpymo_localization_get(&engine)->exit_confirm),
					&cpymo_exit_confirm,
					NULL);
				if (err != CPYMO_ERR_SUCC) {
					printf("[Error] Can not show message box: %s", 
						cpymo_error_message(err));
				}
				#else
				goto EXIT;
				#endif
            }
            case SDL_VIDEOEXPOSE: redraw_system = true; break;
            case SDL_VIDEORESIZE:
                err = set_video_mode(event.resize.w, event.resize.h, current_full_screen);
                if (err != CPYMO_ERR_SUCC) {
                    printf("[Error] SDL_SetVideoMode: %s\n", SDL_GetError());
                    ret = -1;
                    goto EXIT;
                }
                redraw_system = true;
                break;
#ifdef TOGGLE_FULLSCREEN
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT)) {
                    err = set_video_mode(SCREEN_WIDTH, SCREEN_HEIGHT, !current_full_screen);
                    if (err != CPYMO_ERR_SUCC) {
                        printf("[Error] SDL_SetVideoMode: %s\n", SDL_GetError());
                        ret = -1;
                        goto EXIT;
                    }
                }
                break;
#endif
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_WHEELDOWN) {
                    --mouse_wheel;
                }
                else if(event.button.button == SDL_BUTTON_WHEELUP) {
                    ++mouse_wheel;
                }
            };
        }

        bool redraw = false;
        Uint32 cur_time = SDL_GetTicks();
#ifdef REDRAW_WHAT_EVER
        cpymo_engine_request_redraw(&engine);
#endif
        err = cpymo_engine_update(
            &engine, 
            (float)(cur_time - prev_time) / 1000.0f, 
            &redraw);

        if (err != CPYMO_ERR_SUCC) {
            if (err == CPYMO_ERR_NO_MORE_CONTENT) goto EXIT;
            printf("[Error] cpymo_engine_update: %s\n", cpymo_error_message(err));
            ret = -1;
            goto EXIT;
        }

        if (redraw || redraw_system) 
        {
            SDL_FillRect(framebuffer, NULL, 0);
            cpymo_engine_draw(&engine);
            SDL_Flip(framebuffer);
#ifdef REDRAW_WHAT_EVER
            SDL_Delay(16);
#endif
        }
        else 
        {
            SDL_Delay(16);
        }

        prev_time = cur_time;
    }

EXIT:

    cpymo_engine_free(&engine);
    cpymo_backend_image_quit();
    cpymo_backend_audio_free();
    cpymo_backend_font_free();
    SDL_Quit();

    #ifdef LEAKCHECK
    stb_leakcheck_dumpmem();
    #endif

    return ret;
}


