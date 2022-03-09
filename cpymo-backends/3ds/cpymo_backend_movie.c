#include "cpymo_backend_movie.h"

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play() {
	return cpymo_backend_movie_how_to_play_unsupported;
}

error_t cpymo_backend_movie_init(size_t width, size_t height)
{
	return CPYMO_ERR_UNSUPPORTED;
}

void cpymo_backend_movie_free()
{
}

void cpymo_backend_movie_update_yuv_surface(const void *pixels, size_t linesize)
{
}

void cpymo_backend_movie_draw_yuv_surface()
{
}
