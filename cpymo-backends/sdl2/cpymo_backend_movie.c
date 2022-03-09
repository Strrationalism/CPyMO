#include "cpymo_backend_movie.h"
#include <SDL.h>

static SDL_Texture *tex = NULL;
extern SDL_Renderer *renderer;

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play() {
	return cpymo_backend_movie_how_to_play_send_yuv_surface;
}

error_t cpymo_backend_movie_init(size_t width, size_t height)
{
	SDL_assert(tex == NULL);

	tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, (int)width, (int)height);
	if (tex == NULL) return CPYMO_ERR_OUT_OF_MEM;

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_movie_free()
{
	SDL_assert(tex);
	SDL_DestroyTexture(tex);
	tex = NULL;
}

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch)
{
	SDL_UpdateYUVTexture(tex, NULL, (const Uint8 *)y, (int)y_pitch, (const Uint8 *)u, (int)u_pitch, (const Uint8 *)v, (int)v_pitch);
}

void cpymo_backend_movie_draw_yuv_surface()
{
	SDL_RenderCopy(renderer, tex, NULL, NULL);
}
