#ifndef INCLUDE_CPYMO_KEY_HOLD
#define INCLUDE_CPYMO_KEY_HOLD

struct cpymo_engine;

typedef struct {
	bool prev_pressed: 1;
	bool ignore_current: 1;
	bool hold_canceled: 1;
	float timer;
} cpymo_key_hold;

enum cpymo_key_hold_result {
	cpymo_key_hold_result_released,
	cpymo_key_hold_result_just_press,
	cpymo_key_hold_result_pressing,
	cpymo_key_hold_result_just_hold,
	cpymo_key_hold_result_holding,
	cpymo_key_hold_result_just_released,
	cpymo_key_hold_result_hold_released,
	cpymo_key_hold_result_cancel
};

static inline void cpymo_key_hold_init(cpymo_key_hold *h, bool pressed) 
{
	h->prev_pressed = false;
	h->ignore_current = true;
	h->hold_canceled = false;
	h->timer = 0;
}

enum cpymo_key_hold_result cpymo_key_hold_update(
	struct cpymo_engine *e, cpymo_key_hold *h, float dt, bool pressed);

#endif
