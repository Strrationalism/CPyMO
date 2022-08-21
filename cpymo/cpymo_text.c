#include "cpymo_prelude.h"
#include "cpymo_text.h"
#include "cpymo_engine.h"
#include <stdlib.h>
#include <math.h>
#include <assert.h>

void cpymo_text_init(cpymo_text *out)
{
	out->ls = NULL;
	out->active_box = NULL;
}

void cpymo_text_free(cpymo_text *t)
{
	cpymo_text_clear(t);
}

static void cpymo_text_draw_single(struct cpymo_textbox_list *ls, const cpymo_engine *e)
{
	if (ls == NULL) return;
	else {
		cpymo_text_draw_single(ls->next, e);
		cpymo_textbox_draw(e, &ls->box, cpymo_backend_image_draw_type_text_text);
	}
}

void cpymo_text_draw(const cpymo_engine *e)
{
	cpymo_text_draw_single(e->text.ls, e);
}

static bool cpymo_text_wait_fadein(cpymo_engine *e, float dt)
{
	assert(e->text.active_box);
	return cpymo_textbox_wait_text_fadein(e, dt, e->text.active_box);
}

static bool cpymo_text_wait_reading(cpymo_engine *e, float dt)
{
	assert(e->text.active_box);
	return cpymo_textbox_wait_text_reading(e, dt, e->text.active_box);
}

static error_t cpymo_text_callback_read(cpymo_engine *e);

static error_t cpymo_text_callback_fadein(cpymo_engine *e)
{
	cpymo_wait_register_with_callback(
		&e->wait,
		&cpymo_text_wait_reading,
		&cpymo_text_callback_read);
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_text_callback_read(cpymo_engine *e)
{
	cpymo_text *t = &e->text;

	assert(t->active_box);

	if (!cpymo_textbox_all_finished(t->active_box)) {
		error_t err = cpymo_textbox_clear_page(t->active_box, NULL);
		CPYMO_THROW(err);

		cpymo_wait_register_with_callback(
			&e->wait,
			&cpymo_text_wait_fadein,
			&cpymo_text_callback_fadein);
	}
	else {
		t->active_box->msg_cursor_visible = false;
		t->active_box = NULL;
	}

	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}

error_t cpymo_text_new(cpymo_engine *e, float x1, float y1, float x2, float y2, cpymo_color col, float fontsize, cpymo_string text, bool immediately)
{
	struct cpymo_textbox_list *node = (struct cpymo_textbox_list *)malloc(sizeof(struct cpymo_textbox_list));
	if (node == NULL) return CPYMO_ERR_OUT_OF_MEM;

	cpymo_text *t = &e->text;

	node->next = t->ls;

	error_t err = 
		cpymo_textbox_init(&node->box, x1, y1, fabsf(x2 - x1), fabsf(y2 - y1), fontsize, col, text);

	if (err != CPYMO_ERR_SUCC) {
		free(node);
		return err;
	}

	t->ls = node;

	if (immediately) {
		cpymo_textbox_finalize(&node->box);
	}
	else {
		t->active_box = &node->box;
		cpymo_wait_register_with_callback(&e->wait, &cpymo_text_wait_fadein, &cpymo_text_callback_fadein);
	}

	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}

void cpymo_text_clear(cpymo_text *t)
{
	t->active_box = NULL;
	
	struct cpymo_textbox_list *to_free = t->ls;
	while (to_free) {
		struct cpymo_textbox_list *next_to_free = to_free->next;
		cpymo_textbox_free(&to_free->box, NULL);
		free(to_free);
		to_free = next_to_free;
	}

	t->ls = NULL;
}

