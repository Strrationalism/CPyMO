#include "cpymo_charas.h"
#include "cpymo_engine.h"
#include <assert.h>
#include <stdlib.h>

void cpymo_charas_free(cpymo_charas *c)
{
	while (c->chara) {
		struct cpymo_chara *to_free = c->chara;
		c->chara = c->chara->next;
		
		cpymo_backend_image_free(to_free->img);
		free(to_free);
	}

	if (c->anime_owned && c->anime_pos)
		free(c->anime_pos);
}

/*static void print_charas(cpymo_charas *c)
{
	printf("==== charas ====\n");
	struct cpymo_chara *ch = c->chara;
	while (ch) {
		printf("ID: %d, W: %d, H: %d, PTR: %p\n", ch->chara_id, ch->img_w, ch->img_h, ch->img);
		ch = ch->next;
	}
}*/

static bool cpymo_charas_wait_all_tween(cpymo_engine *e, float delta_time)
{
	cpymo_engine_request_redraw(e);
	bool waiting = false;

	bool forward_key_pressed = cpymo_input_foward_key_just_pressed(e);

	struct cpymo_chara ** ppcur = &e->charas.chara;
	struct cpymo_chara * pcur = e->charas.chara;
	while (pcur) {

		if (forward_key_pressed) {
			cpymo_tween_finish(&pcur->pos_x);
			cpymo_tween_finish(&pcur->pos_y);
			cpymo_tween_finish(&pcur->alpha);
		}

		if (!cpymo_tween_finished(&pcur->pos_x)
			|| !cpymo_tween_finished(&pcur->pos_y)
			|| !cpymo_tween_finished(&pcur->alpha))
			waiting = true;

		cpymo_tween_update(&pcur->pos_x, delta_time);
		cpymo_tween_update(&pcur->pos_y, delta_time);
		cpymo_tween_update(&pcur->alpha, delta_time);

		struct cpymo_chara *pnext = pcur->next;

		if (!pcur->alive && cpymo_tween_value(&pcur->alpha) <= 0.0001f) {
			*ppcur = pnext;

			cpymo_backend_image_free(pcur->img);
			free(pcur);
		}
		else {
			ppcur = &pcur->next;
		}

		pcur = pnext;
	}

	return !waiting;
}

void cpymo_charas_draw(const cpymo_engine *e)
{
	const cpymo_charas *c = &e->charas;
	const struct cpymo_chara *pcur = c->chara;
	while (pcur) {
		float anime_offset_x = 0;
		float anime_offset_y = 0;

		if (pcur->play_anime) {
			assert(c->anime_pos_current * 2 + 1 < c->anime_pos_count * 2);
			anime_offset_x = c->anime_pos[c->anime_pos_current * 2] * (float)e->gameconfig.imagesize_w / 540.0f;
			anime_offset_y = c->anime_pos[c->anime_pos_current * 2 + 1] * (float)e->gameconfig.imagesize_h / 360.0f;
		}

		cpymo_backend_image_draw(
			cpymo_tween_value(&pcur->pos_x) + anime_offset_x,
			cpymo_tween_value(&pcur->pos_y) + anime_offset_y,
			(float)pcur->img_w,
			(float)pcur->img_h,
			pcur->img,
			0,
			0,
			pcur->img_w,
			pcur->img_h,
			cpymo_tween_value(&pcur->alpha),
			cpymo_backend_image_draw_type_chara);

		pcur = pcur->next;
	}
}

error_t cpymo_chara_convert_to_mode0_pos(
	cpymo_engine *e,
	struct cpymo_chara *c,
	int coord_mode,
	float *x, float *y)
{
	if (coord_mode < 0 || coord_mode > 6)
		return CPYMO_ERR_INVALID_ARG;

	if (coord_mode == 0 || coord_mode == 4) {
		// 立绘左沿距屏幕左沿的距离
	}
	else if (coord_mode == 1 || coord_mode == 3 || coord_mode == 5) {
		// 立绘中点距屏幕左沿的距离
		*x -= (float)c->img_w / 2.0f;
	}
	else if (coord_mode == 2 || coord_mode == 6) {
		// 立绘右沿距屏幕右沿的距离
		*x = (float)e->gameconfig.imagesize_w - *x - (float)c->img_w;
	}

	if (coord_mode <= 2) {
		// 立绘上沿距屏幕上沿的距离
	}
	else if (coord_mode == 3) {
		// 立绘中点距屏幕上沿的距离
		*y -= (float)c->img_h / 2.0f;
	}
	else if (coord_mode >= 4) {
		// 立绘下沿距屏幕下沿的距离
		*y = (float)e->gameconfig.imagesize_h - *y - (float)c->img_h;
	}

	return CPYMO_ERR_SUCC;
}

error_t cpymo_charas_new_chara(
	cpymo_engine *e, 
	struct cpymo_chara **out, 
	cpymo_parser_stream_span filename, 
	int chara_id, int layer, 
	int coord_mode, float x, float y, 
	float begin_alpha, float time)
{
	
	struct cpymo_chara *ch = NULL;
	error_t err = cpymo_charas_find(&e->charas, &ch, chara_id);
	if (err == CPYMO_ERR_SUCC) {
		ch->alive = false;
		cpymo_tween_to(&ch->alpha, 0, time);
	}

	ch = (struct cpymo_chara *)malloc(sizeof(struct cpymo_chara));
	if (ch == NULL) return CPYMO_ERR_OUT_OF_MEM;

	err = cpymo_assetloader_load_chara_image(&ch->img, &ch->img_w, &ch->img_h, filename, &e->assetloader);
	if (err != CPYMO_ERR_SUCC) {
		free(ch);
		return err;
	}

	err = cpymo_chara_convert_to_mode0_pos(e, ch, coord_mode, &x, &y);
	if (err != CPYMO_ERR_SUCC) {
		cpymo_backend_image_free(ch->img);
		free(ch);
		return err;
	}

	ch->play_anime = false;
	ch->chara_id = chara_id;
	ch->layer = layer;
	ch->alive = true;
	cpymo_tween_assign(&ch->pos_x, x);
	cpymo_tween_assign(&ch->pos_y, y);
	cpymo_tween_assign(&ch->alpha, begin_alpha);
	cpymo_tween_to(&ch->alpha, 1.0f, time);

	struct cpymo_chara *prev = e->charas.chara;
	while (prev) {
		struct cpymo_chara *next = prev->next;
		if (next == NULL) break;

		if (next->layer >= layer && prev->layer <= layer)
			break;
		else
			prev = next;
	}

	if (prev) {
		ch->next = prev->next;
		prev->next = ch;
	}
	else {
		ch->next = NULL;
		e->charas.chara = ch;
	}
	

	*out = ch;

	//print_charas(&e->charas);

	return CPYMO_ERR_SUCC;
}

error_t cpymo_charas_find(cpymo_charas * c, struct cpymo_chara ** out, int chara_id)
{
	assert(*out == NULL);

	struct cpymo_chara *pcur = c->chara;
	while (pcur) {
		if (pcur->chara_id == chara_id && pcur->alive) {
			*out = pcur;
			return CPYMO_ERR_SUCC;
		}

		pcur = pcur->next;
	}

	return CPYMO_ERR_NOT_FOUND;
}

error_t cpymo_charas_kill(cpymo_engine *e, int chara_id, float time)
{
	struct cpymo_chara *ch = NULL;
	error_t err = cpymo_charas_find(&e->charas, &ch, chara_id);
	CPYMO_THROW(err);

	ch->alive = false;
	cpymo_tween_to(&ch->alpha, 0, time);

	return CPYMO_ERR_SUCC;
}

void cpymo_charas_wait(cpymo_engine *e)
{
	cpymo_wait_register(&e->wait, &cpymo_charas_wait_all_tween);
}

void cpymo_charas_kill_all(cpymo_engine *e, float time)
{
	struct cpymo_chara *ch = e->charas.chara;
	while (ch) {
		if (ch->alive) {
			ch->alive = false;
			cpymo_tween_to(&ch->alpha, 0, time);
		}

		ch = ch->next;
	}
}

void cpymo_charas_fast_kill_all(cpymo_charas * c)
{
	cpymo_charas_free(c);
	cpymo_charas_init(c);
}

error_t cpymo_charas_pos(cpymo_engine *e, int chara_id, int coord_mode, float x, float y)
{
	struct cpymo_chara *c;
	error_t err = cpymo_charas_find(&e->charas, &c, chara_id);
	CPYMO_THROW(err);

	err = cpymo_chara_convert_to_mode0_pos(e, c, coord_mode, &x, &y);
	CPYMO_THROW(err);

	cpymo_tween_assign(&c->pos_x, x);
	cpymo_tween_assign(&c->pos_y, y);

	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}

void cpymo_charas_stop_all_anime(cpymo_charas *c)
{
	struct cpymo_chara *ch = c->chara;
	
	float last_pos_x = c->anime_pos[(c->anime_pos_count - 1) * 2];
	float last_pos_y = c->anime_pos[(c->anime_pos_count - 1) * 2 + 1];

	while (ch) {
		ch->play_anime = false;
		cpymo_tween_assign(&ch->pos_x, last_pos_x + cpymo_tween_value(&ch->pos_x));
		cpymo_tween_assign(&ch->pos_y, last_pos_y + cpymo_tween_value(&ch->pos_y));

		ch = ch->next;
	}

	if (c->anime_owned && c->anime_pos)
		free(c->anime_pos);

	c->anime_owned = false;
	c->anime_pos = NULL;
	c->anime_pos_current = 0;
	c->anime_loop = 0;
}

void cpymo_charas_set_play_anime(cpymo_charas * c, int id)
{
	struct cpymo_chara *ch = NULL;
	if (cpymo_charas_find(c, &ch, id) == CPYMO_ERR_SUCC)
		ch->play_anime = true;
}

static error_t cpymo_charas_anime_finished_callback(cpymo_engine *e)
{
	cpymo_engine_request_redraw(e);
	cpymo_charas_stop_all_anime(&e->charas);
	return CPYMO_ERR_SUCC;
}

static bool cpymo_charas_wait_for_anime(cpymo_engine *e, float delta_time)
{
	if (cpymo_input_foward_key_just_pressed(e)) 
		return true;

	cpymo_charas *c = &e->charas;

	c->anime_timer += delta_time;
	while (c->anime_timer >= c->anime_period) {
		cpymo_engine_request_redraw(e);

		c->anime_timer -= c->anime_period;
		c->anime_pos_current++;

		if (c->anime_pos_current >= c->anime_pos_count) {
			c->anime_pos_current = 0;
			c->anime_loop--;
		}
	}

	return c->anime_loop <= 0;
}

void cpymo_charas_play_anime(
	cpymo_engine *e,
	float period,
	int loop_num,
	float *offsets,
	size_t offsets_xy_count,
	bool offsets_owned)
{
	cpymo_charas *c = &e->charas;
	assert(c->anime_pos == NULL);

	cpymo_engine_request_redraw(e);

	c->anime_period = period;
	c->anime_pos_current = 0;
	c->anime_pos_count = offsets_xy_count;
	c->anime_pos = offsets;
	c->anime_loop = loop_num;
	c->anime_owned = offsets_owned;
	c->anime_timer = 0;

	cpymo_wait_register_with_callback(
		&e->wait, 
		&cpymo_charas_wait_for_anime,
		&cpymo_charas_anime_finished_callback);
}
