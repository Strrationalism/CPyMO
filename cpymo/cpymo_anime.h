#ifndef INCLUDE_CPYMO_ANIME
#define INCLUDE_CPYMO_ANIME

#include <cpymo_backend_image.h>
#include "cpymo_parser.h"

struct cpymo_engine;

typedef struct {
	cpymo_backend_image anime_image;
	float interval;
	float current_time;
	int current_frame;
	int frame_height;
	int all_frame;
	bool is_loop;
	float draw_x, draw_y;
	int image_width;

	char *anime_name;
} cpymo_anime;

void cpymo_anime_update(
	struct cpymo_engine *e, 
	cpymo_anime *anime, 
	float delta_time);
	
void cpymo_anime_draw(const cpymo_anime *);

error_t cpymo_anime_on(
	struct cpymo_engine *engine,
	int frames, 
	cpymo_str filename, 
	float x, 
	float y, 
	float interval, 
	bool loop);

void cpymo_anime_off(cpymo_anime *anime);

static inline void cpymo_anime_init(cpymo_anime *anime)
{ anime->anime_image = NULL; anime->anime_name = NULL; }

static inline void cpymo_anime_free(cpymo_anime *anime)
{ cpymo_anime_off(anime); }

#endif