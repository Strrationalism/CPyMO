#ifndef INCLUDE_CPYMO_BACKEND_MOVIE
#define INCLUDE_CPYMO_BACKEND_MOVIE

#include <cpymo_error.h>
#include <stddef.h>

enum cpymo_backend_movie_how_to_play {
	cpymo_backend_movie_how_to_play_unsupported,
	cpymo_backend_movie_how_to_play_host_player,
	cpymo_backend_movie_how_to_play_send_yuv_surface
};

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play();

error_t cpymo_backend_movie_init(size_t width, size_t height);
void cpymo_backend_movie_free();

void cpymo_backend_movie_update_yuv_surface(const void *pixels, size_t linesize);
void cpymo_backend_movie_draw_yuv_surface();


#endif
