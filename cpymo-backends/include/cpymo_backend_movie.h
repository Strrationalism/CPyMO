#ifndef INCLUDE_CPYMO_BACKEND_MOVIE
#define INCLUDE_CPYMO_BACKEND_MOVIE

#include "../../cpymo/cpymo_error.h"
#include <stddef.h>

enum cpymo_backend_movie_how_to_play {
	cpymo_backend_movie_how_to_play_unsupported,
	cpymo_backend_movie_how_to_play_send_surface
};

enum cpymo_backend_movie_format {
	cpymo_backend_movie_format_yuv420p,
	cpymo_backend_movie_format_yuv422p,
	cpymo_backend_movie_format_yuv420p16,
	cpymo_backend_movie_format_yuv422p16,
	cpymo_backend_movie_format_yuyv422,
};

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play();

error_t cpymo_backend_movie_init_surface(size_t width, size_t height, enum cpymo_backend_movie_format format);
void cpymo_backend_movie_free_surface();

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch);

void cpymo_backend_movie_update_yuyv_surface(const void *, size_t pitch);

void cpymo_backend_movie_draw_surface();


#endif
