#ifndef INCLUDE_CPYMO_KEY_HOLD
#define INCLUDE_CPYMO_KEY_HOLD

typedef struct {
	bool prev_pressed: 1;
	bool ignore_current: 1;
	float timer;
} cpymo_key_hold;

enum cpymo_key_hold_result {
	cpymo_key_hold_result_released,
	cpymo_key_hold_result_just_press,
	cpymo_key_hold_result_pressing,
	cpymo_key_hold_result_just_hold,
	cpymo_key_hold_result_holding,
	cpymo_key_hold_result_just_released,
	cpymo_key_hold_result_hold_released
};

static inline void cpymo_key_hold_init(cpymo_key_hold *h, bool pressed) 
{
	h->prev_pressed = false;
	h->ignore_current = true;
	h->timer = 0;
}

static inline enum cpymo_key_hold_result cpymo_key_hold_update(cpymo_key_hold *h, float dt, bool pressed)
{
	if (!pressed) h->ignore_current = false;
	if (h->ignore_current) return cpymo_key_hold_result_released;

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
		if (h->timer - dt < 0.3f && h->timer >= 0.3f) 
			return cpymo_key_hold_result_just_hold;
		else if (h->timer >= 0.3f) return cpymo_key_hold_result_holding;
		else return cpymo_key_hold_result_pressing;
	}
	else {//if (!pressed && prev_pressed) {
		if (h->timer >= 0.3f) return cpymo_key_hold_result_hold_released;
		else return cpymo_key_hold_result_just_released;
	}
}

#endif
