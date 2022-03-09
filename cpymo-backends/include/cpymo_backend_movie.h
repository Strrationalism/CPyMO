#ifndef INCLUDE_CPYMO_BACKEND_MOVIE
#define INCLUDE_CPYMO_BACKEND_MOVIE

#include <cpymo_error.h>

error_t cpymo_backend_movie_player_init();
void cpymo_backend_movie_player_free();

error_t cpymo_backend_movie_player_draw_yuv(
	const void *y, int ypitch, 
	const void *u, int upitch, 
	const void *v, int vpitch);

#endif
