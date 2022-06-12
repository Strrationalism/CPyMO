#ifndef INCLUDE_CPYMO_WAIT
#define INCLUDE_CPYMO_WAIT

#include "cpymo_error.h"
#include <stdbool.h>
#include <stddef.h>

struct cpymo_engine;

typedef bool (*cpymo_wait_for)(struct cpymo_engine *, float);	// wait until it's returns true.
typedef error_t (*cpymo_wait_over_callback)(struct cpymo_engine *);	// You can register next wait operation in callback.

typedef struct {
	cpymo_wait_for wating_for;
	cpymo_wait_over_callback callback;

	float wait_for_seconds;
} cpymo_wait;

static inline void cpymo_wait_reset(cpymo_wait *wait)
{
	wait->callback = NULL;
	wait->wating_for = NULL;
}

static inline bool cpymo_wait_is_wating(cpymo_wait *wait)
{
	return wait->wating_for != NULL;
}

void cpymo_wait_register_with_callback(cpymo_wait *wait, cpymo_wait_for wait_for, cpymo_wait_over_callback cb);

static inline void cpymo_wait_register(cpymo_wait *wait, cpymo_wait_for wait_for)
{
	cpymo_wait_register_with_callback(wait, wait_for, NULL);
}

error_t cpymo_wait_update(cpymo_wait *wait, struct cpymo_engine *engine, float delta_time);

void cpymo_wait_for_seconds(cpymo_wait *, float seconds);
void cpymo_wait_callback_after_seconds(cpymo_wait *, float seconds, cpymo_wait_over_callback);
void cpymo_wait_callback_nextframe(cpymo_wait *, cpymo_wait_over_callback);

#endif