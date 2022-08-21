#include "cpymo_prelude.h"
#include "cpymo_anime.h"
#include "cpymo_engine.h"
#include <stb_image.h>
#include <string.h>
void cpymo_anime_draw(const cpymo_anime *anime)
{
	if (anime->anime_image) {
		cpymo_backend_image_draw(
			anime->draw_x, anime->draw_y,
			(float)anime->image_width, (float)anime->frame_height,
			anime->anime_image,
			0, anime->frame_height * anime->current_frame,
			anime->image_width, anime->frame_height,
			1.0f,
			cpymo_backend_image_draw_type_effect
		);
	}
}

error_t cpymo_anime_on(
	struct cpymo_engine *engine,
	int frames,
	cpymo_string filename_span,
	float x,
	float y,
	float interval,
	bool loop)
{
	cpymo_anime_off(&engine->anime);

	cpymo_backend_image img;
	int w, h;
	error_t err = cpymo_assetloader_load_system_image(
		&img,
		&w, &h,
		filename_span,
		&engine->assetloader,
		true);
	if (err != CPYMO_ERR_SUCC) return err;

	engine->anime.anime_image = img;
	engine->anime.interval = interval;
	engine->anime.current_time = interval;
	engine->anime.current_frame = -1;
	engine->anime.frame_height = h / frames;
	engine->anime.all_frame = frames;
	engine->anime.is_loop = loop;
	engine->anime.draw_x = x;
	engine->anime.draw_y = y;
	engine->anime.image_width = w;

	if (loop) {
		char *anime_name = (char *)realloc(engine->anime.anime_name, filename_span.len + 1);
		if (anime_name) {
			cpymo_string_copy(anime_name, filename_span.len + 1, filename_span);
			engine->anime.anime_name = anime_name;
		}
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_anime_update(cpymo_anime *anime, float delta_time, bool * redraw)
{
	if (anime->anime_image) {
		anime->current_time += delta_time;
		if (anime->current_time >= anime->interval) {
			anime->current_time = 0.0f;
			*redraw = true;

			anime->current_frame++;
			if (anime->current_frame >= anime->all_frame) {
				if (anime->is_loop) anime->current_frame = 0;
				else cpymo_anime_off(anime);
			}
		}
	}
}

void cpymo_anime_off(cpymo_anime *anime)
{
	if (anime->anime_name) {
		free(anime->anime_name);
		anime->anime_name = NULL;
	}

	if (anime->anime_image) {
		cpymo_backend_image_free(anime->anime_image);
		anime->anime_image = NULL;
	}
}
