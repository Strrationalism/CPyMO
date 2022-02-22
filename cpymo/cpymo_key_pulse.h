#ifndef INCLUDE_CPYMO_KEY_PULSE
#define INCLUDE_CPYMO_KEY_PULSE

#include <stdbool.h>

typedef struct {
	unsigned state : 1;
	bool prev_pressed : 1;
	bool output : 1;
	float timer;
} cpymo_key_pluse;

static inline void cpymo_key_pluse_init(cpymo_key_pluse *p, bool current_pressed)
{
	p->state = 0;
	p->output = false;
	p->timer = 0;
	p->prev_pressed = current_pressed;
}

static inline void cpymo_key_pluse_update(cpymo_key_pluse *p, float dt, bool pressed) 
{
	p->output = false;
	if (pressed) {
		p->timer += dt;
		if (p->state == 0) {
			if (!p->prev_pressed && pressed) {
				p->output = true;
			}
			else {
				if (p->timer >= 0.5f) {
					p->state = 1;
					p->timer -= 0.5f;
					p->output = true;
				}
			}
		}
		else {
			if (p->timer >= 0.075f) {
				p->output = true;
				p->timer -= 0.075f;
			}
		}
	}
	else {
		p->state = 0;
		p->output = false;
		p->timer = 0;
	}

	p->prev_pressed = pressed;
}

static inline bool cpymo_key_pluse_output(cpymo_key_pluse *p)
{ return p->output; }

#endif
