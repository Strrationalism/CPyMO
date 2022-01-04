#ifndef INCLUDE_CPYMO_TWEEN
#define INCLUDE_CPYMO_TWEEN

typedef struct {
	float all_time, current_time;
} cpymo_tween;

static inline cpymo_tween cpymo_tween_create(float all_time)
{
	cpymo_tween t;
	t.all_time = all_time;
	t.current_time = 0;
	return t;
}

static inline void cpymo_tween_update(cpymo_tween *tween, float time)
{ tween->current_time += time; }

static inline float cpymo_tween_progress(const cpymo_tween *tween)
{ return tween->current_time / tween->all_time; }

static inline float cpymo_tween_finished(const cpymo_tween *tween)
{ return tween->current_time >= tween->all_time; }

static inline void cpymo_tween_finish(cpymo_tween *tween)
{ tween->current_time = tween->all_time; }

#endif
