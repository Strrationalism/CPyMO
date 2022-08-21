#include "cpymo_prelude.h"
#include "cpymo_scroll.h"
#include "cpymo_utils.h"
#include "cpymo_engine.h"
#include <stdlib.h>
#include <assert.h>

void cpymo_scroll_draw(const cpymo_scroll * s)
{
	if (s->img) {
		float t = s->time / s->all_time;
		assert(t >= 0 && t <= 1);

		cpymo_backend_image_draw(
			cpymo_utils_lerp(s->sx, s->ex, t),
			cpymo_utils_lerp(s->sy, s->ey, t),
			(float)s->w,
			(float)s->h,
			s->img,
			0, 0, s->w, s->h,
			1.0f,
			cpymo_backend_image_draw_type_bg
		);
	}
}

static bool cpymo_scroll_wait(cpymo_engine *e, float delta_time)
{
	cpymo_engine_request_redraw(e);
	e->scroll.time += delta_time;

	if (cpymo_input_foward_key_just_released(e)) 
		e->scroll.time = e->scroll.all_time;

	return e->scroll.time >= e->scroll.all_time;
}

static error_t cpymo_scroll_callback(cpymo_engine *e)
{
	cpymo_engine_request_redraw(e);
	e->scroll.time = e->scroll.all_time;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_scroll_start(cpymo_engine *e, cpymo_str bgname, float sx, float sy, float ex, float ey, float time)
{
	cpymo_bg_reset(&e->bg);
	cpymo_charas_fast_kill_all(&e->charas);

	char *next_bg_name = (char *)realloc(e->bg.current_bg_name, bgname.len + 1);
	if (next_bg_name) {
		cpymo_str_copy(next_bg_name, bgname.len + 1, bgname);
		e->bg.current_bg_name = next_bg_name;
	}

	cpymo_scroll *s = &e->scroll;
	cpymo_scroll_reset(s);

	error_t err = cpymo_assetloader_load_bg_image(&s->img, &s->w, &s->h, bgname, &e->assetloader);
	if (err != CPYMO_ERR_SUCC) {
		s->img = NULL;
		return err;
	}

	s->sx = -sx / 100.0f * (float)s->w;
	s->sy = -sy / 100.0f * (float)s->h;
	s->ex = -ex / 100.0f * (float)s->w;
	s->ey = -ey / 100.0f * (float)s->h;
	s->time = 0;
	s->all_time = time;

	cpymo_wait_register_with_callback(&e->wait, cpymo_scroll_wait, cpymo_scroll_callback);
	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}


