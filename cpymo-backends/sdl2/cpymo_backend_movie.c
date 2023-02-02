#include <stdbool.h>
bool playing_movie = false;

#ifndef DISABLE_MOVIE
#include <cpymo_prelude.h>
#include "cpymo_backend_movie.h"
#include "cpymo_import_sdl2.h"
#include <cpymo_engine.h>

static SDL_Texture *tex = NULL;
extern SDL_Renderer *renderer;
extern cpymo_engine engine;
#if SDL_VERSION_ATLEAST(2, 0, 10)
static SDL_FRect draw_target;
#else
static SDL_Rect draw_target;
#endif


enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play() {
	return cpymo_backend_movie_how_to_play_send_surface;
}

error_t cpymo_backend_movie_init_surface(size_t width, size_t height, enum cpymo_backend_movie_format format)
{
	SDL_assert(tex == NULL);

	int renderer_w, renderer_h;
	if (SDL_GetRendererOutputSize(renderer, &renderer_w, &renderer_h)) {
		renderer_w = (int)engine.gameconfig.imagesize_w;
		renderer_h = (int)engine.gameconfig.imagesize_h; 
	}
	
	float ratio_w = (float)renderer_w / (float)width;
	float ratio_h = (float)renderer_h / (float)height;
	float ratio = ratio_w < ratio_h ? ratio_w : ratio_h;
	draw_target.w = 
		#if (!SDL_VERSION_ATLEAST(2, 0, 10))
		(int)
		#endif
		(ratio * (float)width);

	draw_target.h = 
		#if (!SDL_VERSION_ATLEAST(2, 0, 10))
		(int)
		#endif
		(ratio * (float)height);

	draw_target.x = 
		#if (!SDL_VERSION_ATLEAST(2, 0, 10))
		(int)
		#endif
		(((float)renderer_w - draw_target.w) / 2);

	draw_target.y = 
		#if (!SDL_VERSION_ATLEAST(2, 0, 10))
		(int)
		#endif
		(((float)renderer_h - draw_target.h) / 2);

	int sdlfmt;
	switch (format) {
	case cpymo_backend_movie_format_yuv420p: 
	case cpymo_backend_movie_format_yuv422p: 
	case cpymo_backend_movie_format_yuv420p16:
	case cpymo_backend_movie_format_yuv422p16:
		sdlfmt = SDL_PIXELFORMAT_IYUV; break;
	case cpymo_backend_movie_format_yuyv422:
		sdlfmt = SDL_PIXELFORMAT_YUY2; break;
	default: return CPYMO_ERR_UNSUPPORTED;
	};

	tex = SDL_CreateTexture(renderer, sdlfmt, SDL_TEXTUREACCESS_STREAMING, (int)width, (int)height);
	if (tex == NULL) return CPYMO_ERR_OUT_OF_MEM;

	playing_movie = true;
	SDL_RenderSetLogicalSize(renderer, (int)renderer_w, (int)renderer_h);

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_movie_update_yuyv_surface(const void *p, size_t pitch)
{
	SDL_UpdateTexture(tex, NULL, p, (int)pitch);
}

void cpymo_backend_movie_free_surface()
{
	SDL_assert(tex);
	SDL_DestroyTexture(tex);
	tex = NULL;
	playing_movie = false;

	SDL_RenderSetLogicalSize(
		renderer, 
		(int)engine.gameconfig.imagesize_w, 
		(int)engine.gameconfig.imagesize_h);
}

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch)
{
	SDL_UpdateYUVTexture(tex, NULL, (const Uint8 *)y, (int)y_pitch, (const Uint8 *)u, (int)u_pitch, (const Uint8 *)v, (int)v_pitch);
}

void cpymo_backend_movie_draw_surface()
{
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_RenderCopyF(renderer, tex, NULL, &draw_target);
#else
	SDL_RenderCopy(renderer, tex, NULL, &draw_target);
#endif
}

#endif
