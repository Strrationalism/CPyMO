#include "cpymo_anime.h"
#include "cpymo_engine.h"
#include <stb_image.h>

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
	cpymo_parser_stream_span filename_span,
	float x,
	float y,
	float interval,
	bool loop)
{
	cpymo_anime_off(&engine->anime);

	char filename[128];
	cpymo_parser_stream_span_copy(filename, sizeof(filename), filename_span);
	char *buf = NULL;
	size_t buf_size = 0;
	error_t err = cpymo_assetloader_load_system(&buf, &buf_size, filename, "png", &engine->assetloader);
	if (err != CPYMO_ERR_SUCC) return err;

	int w, h, c;
	stbi_uc *px = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &w, &h, &c, 4);
	free(buf);

	if (px == NULL) return CPYMO_ERR_BAD_FILE_FORMAT;

	cpymo_backend_image img;
	err = cpymo_backend_image_load_immutable(&img, px, w, h, cpymo_backend_image_format_rgba);

	if (err != CPYMO_ERR_SUCC) {
		free(px);
		return err;
	}

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
	if (anime->anime_image) {
		cpymo_backend_image_free(anime->anime_image);
		anime->anime_image = NULL;
	}
}