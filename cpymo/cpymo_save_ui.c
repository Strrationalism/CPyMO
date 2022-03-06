#include "cpymo_save_ui.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"

#define MAX_SAVES 10

typedef struct {
	bool is_load_ui;
} cpymo_save_ui;

static void cpymo_save_ui_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{

}

static error_t cpymo_save_ui_ok(cpymo_engine *e, void *selected)
{
	return CPYMO_ERR_SUCC;
}

static void cpymo_save_ui_deleter(cpymo_engine *e, void *ui_data)
{

}

static void* cpymo_save_ui_get_next(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t i = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(cur);
	if (i < MAX_SAVES) return CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(i + 1);
	else return NULL;
}

static void* cpymo_save_ui_get_prev(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t i = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(cur);
	if (i == 0) return NULL;
	else return CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(i + 1);
}

error_t cpymo_save_ui_enter(cpymo_engine *e, bool is_load_ui)
{
	cpymo_save_ui *ui = NULL;
	cpymo_list_ui_enter(
		e,
		(void **)&ui,
		sizeof(*ui),
		&cpymo_save_ui_draw_node,
		&cpymo_save_ui_ok,
		&cpymo_save_ui_deleter,
		CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(0),
		&cpymo_save_ui_get_next,
		&cpymo_save_ui_get_prev,
		false,
		3);

	ui->is_load_ui = is_load_ui;

	return CPYMO_ERR_SUCC;
}
