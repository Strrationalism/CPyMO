#ifndef INCLUDE_CPYMO_KEY_HOLD
#define INCLUDE_CPYMO_KEY_HOLD

typedef struct {
	bool prev_pressed;
	float timer;
} cpymo_key_hold;

enum cpymo_key_hold_result {
	cpymo_key_hold_result_released,
	cpymo_key_hold_result_just_press,
	cpymo_key_hold_result_pressing,
	cpymo_key_hold_result_holding,
	cpymo_key_hold_result_just_released,
	cpymo_key_hold_result_hold_released
};

static inline void cpymo_key_hold_init(cpymo_key_hold *h, bool pressed) 
{
	h->prev_pressed = pressed;
	h->timer = 0;
}

static inline enum cpymo_key_hold_result cpymo_key_hold_update(cpymo_key_hold *h, float dt, bool pressed)
{
	const bool prev_pressed = h->prev_pressed;
	h->prev_pressed = pressed;

	if (!pressed && !prev_pressed) {
		h->timer = 0;
		return cpymo_key_hold_result_released;
	}
	else if (pressed && !prev_pressed) {
		h->timer = 0;
		return cpymo_key_hold_result_just_press;
	}
	else if (pressed && prev_pressed) {
		h->timer += dt;
		if (h->timer >= 0.6f) return cpymo_key_hold_result_holding;
		else return cpymo_key_hold_result_pressing;
	}
	else {//if (!pressed && prev_pressed) {
		if (h->timer >= 0.6f) return cpymo_key_hold_result_hold_released;
		else return cpymo_key_hold_result_just_released;
	}
}

#endif
