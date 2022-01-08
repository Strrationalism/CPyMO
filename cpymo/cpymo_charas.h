#ifndef INCLUDE_CPYMO_CHARAS
#define INCLUDE_CPYMO_CHARAS

#include <cpymo_backend_image.h>
#include "cpymo_tween.h"
#include "cpymo_parser.h"

struct cpymo_engine;

struct cpymo_chara {
	int chara_id, layer;
	
	cpymo_backend_image img;
	bool alive;
	cpymo_tween pos_x, pos_y, alpha;
	int img_w, img_h;

	bool play_anime;

	struct cpymo_chara *next;
};

typedef struct {
	struct cpymo_chara *chara;

	float *anime_pos;
	int anime_loop;
	float anime_period;
	float anime_timer;
	size_t anime_pos_count, anime_pos_current;
	bool anime_owned;
} cpymo_charas;

static inline void cpymo_charas_init(cpymo_charas *cpymo_charas)
{ 
	cpymo_charas->chara = NULL;
	cpymo_charas->anime_pos = NULL;
	cpymo_charas->anime_owned = false;
	cpymo_charas->anime_pos_current = 0;
}

void cpymo_charas_free(cpymo_charas *);

void cpymo_charas_draw(const struct cpymo_engine *);

error_t cpymo_charas_new_chara(
	struct cpymo_engine *, struct cpymo_chara **out,
	cpymo_parser_stream_span filename,
	int chara_id, int layer,
	int coord_mode,
	float x, float y,
	float begin_alpha, float time);

error_t cpymo_charas_find(
	cpymo_charas *, struct cpymo_chara **out,
	int chara_id);

error_t cpymo_charas_kill(
	struct cpymo_engine *,
	int chara_id,
	float time);

error_t cpymo_chara_convert_to_mode0_pos(
	struct cpymo_engine *e,
	struct cpymo_chara *c,
	int coord_mode,
	float *x, float *y);

void cpymo_charas_wait(struct cpymo_engine *);

void cpymo_charas_kill_all(struct cpymo_engine *, float time);

void cpymo_charas_fast_kill_all(cpymo_charas *c);

error_t cpymo_charas_pos(
	struct cpymo_engine *,
	int chara_id, 
	int coord_mode, 
	float x, float y);

void cpymo_charas_set_play_anime(cpymo_charas *c, int id);
void cpymo_charas_play_anime(
	struct cpymo_engine *e, 
	float period, 
	int loop_num, 
	float *offsets, 
	size_t offsets_xy_count,
	bool offsets_owned);

#endif