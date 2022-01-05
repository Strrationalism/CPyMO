#include "cpymo_wait.h"
#include "cpymo_engine.h"
#include <assert.h>

void cpymo_wait_register_with_callback(cpymo_wait * wait, cpymo_wait_for wait_for, cpymo_wait_over_callback cb)
{
	assert(wait->wating_for == NULL);
	assert(wait->callback == NULL);

	wait->wating_for = wait_for;
	wait->callback = cb;
}

error_t cpymo_wait_update(cpymo_wait *wait, cpymo_engine * engine, float delta_time)
{
	error_t err = CPYMO_ERR_SUCC;
	if (cpymo_wait_is_wating(wait)) {
		if (wait->wating_for(engine, delta_time)) {
			cpymo_wait_over_callback cb = wait->callback;
			cpymo_wait_reset(wait);

			if (cb) err = cb(engine);
		}
	}

	return err;
}

static bool cpymo_wait_second_waiter(struct cpymo_engine *e, float delta_time)
{
	if(cpymo_input_foward_key_just_pressed(e))
		e->wait.wait_for_seconds = -1;

	e->wait.wait_for_seconds -= delta_time;
	return e->wait.wait_for_seconds <= 0;
}

void cpymo_wait_for_seconds(cpymo_engine * engine, float seconds)
{
	engine->wait.wait_for_seconds = seconds;
	cpymo_wait_register(&engine->wait, &cpymo_wait_second_waiter);
}

void cpymo_wait_callback_after_seconds(cpymo_engine * engine, float seconds, cpymo_wait_over_callback cb)
{
	engine->wait.wait_for_seconds = seconds;
	cpymo_wait_register_with_callback(&engine->wait, &cpymo_wait_second_waiter, cb);
}
