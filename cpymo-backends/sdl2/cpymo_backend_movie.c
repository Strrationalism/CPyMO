#include "cpymo_backend_movie.h"
#include "cpymo_import_sdl2.h"
#include <cpymo_engine.h>

static SDL_Texture *tex = NULL;
extern SDL_Renderer *renderer;
extern cpymo_engine engine;

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play() {
	return cpymo_backend_movie_how_to_play_send_surface;
}

error_t cpymo_backend_movie_init_surface(size_t width, size_t height, enum cpymo_backend_movie_format format)
{
	SDL_assert(tex == NULL);

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

	SDL_RenderSetLogicalSize(renderer, (int)width, (int)height);

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
	SDL_RenderSetLogicalSize(renderer, engine.gameconfig.imagesize_w, engine.gameconfig.imagesize_h);
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
	SDL_RenderCopy(renderer, tex, NULL, NULL);
}
