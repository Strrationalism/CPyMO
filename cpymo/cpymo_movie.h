#ifndef INCLUDE_CPYMO_MOVIE
#define INCLUDE_CPYMO_MOVIE

#include "cpymo_error.h"

struct cpymo_engine;

error_t cpymo_movie_play(struct cpymo_engine *e, const char *path);

#endif
