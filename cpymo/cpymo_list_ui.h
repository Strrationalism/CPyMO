#ifndef INCLUDE_CPYMO_LIST_UI
#define INCLUDE_CPYMO_LIST_UI

#include "cpymo_ui.h"
#include "cpymo_key_pulse.h"
#include "cpymo_key_hold.h"
#include <stdint.h>

#define CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(INDEX) ((void *)((INDEX) + 1))
#define CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(PTR) (((uintptr_t)(PTR)) - 1)

typedef void *(*cpymo_list_ui_get_node)(const struct cpymo_engine *, const void *ui_data, const void *cur);
typedef void (*cpymo_list_ui_draw_node)(const struct cpymo_engine *, const void *node_to_draw, float y);
typedef error_t (*cpymo_list_ui_ok)(struct cpymo_engine *, void *selected);
typedef error_t(*cpymo_list_ui_custom_update)(struct cpymo_engine *, float dt, void *selected);
typedef error_t(*cpymo_list_ui_selecting_no_more_content_callback)(struct cpymo_engine *, bool is_down);
typedef error_t(*cpymo_list_ui_selection_changed)(struct cpymo_engine *e, void *selected);
typedef error_t(*cpymo_list_ui_mouse_button_just_hold_callback)(struct cpymo_engine *e);

error_t cpymo_list_ui_enter(
	struct cpymo_engine *,
	void **out_ui_data,
	size_t ui_data_size,
	cpymo_list_ui_draw_node draw_node,
	cpymo_list_ui_ok ok,
	cpymo_ui_deleter ui_data_deleter,
	void *current,
	cpymo_list_ui_get_node get_next,
	cpymo_list_ui_get_node get_prev,
	bool from_bottom_to_top,
	size_t nodes_per_screen);

void cpymo_list_ui_set_allow_exit(struct cpymo_engine *e, bool b);

void cpymo_list_ui_set_selection_relative_to_cur(struct cpymo_engine *e, int c);

void cpymo_list_ui_set_current_node(struct cpymo_engine *e, void *cur);

void cpymo_list_ui_set_selection_changed_callback(struct cpymo_engine *e, cpymo_list_ui_selection_changed c);

void cpymo_list_ui_set_scroll_enabled(struct cpymo_engine *e, bool allow_scroll);

void cpymo_list_ui_set_custom_update(struct cpymo_engine *e, cpymo_list_ui_custom_update update);

void *cpymo_list_ui_data(struct cpymo_engine *e);

void cpymo_list_ui_set_selecting_no_more_content_callback(struct cpymo_engine *e, cpymo_list_ui_selecting_no_more_content_callback c);

void cpymo_list_ui_set_mouse_button_just_hold_callback(struct cpymo_engine *e, cpymo_list_ui_mouse_button_just_hold_callback c);

const void *cpymo_list_ui_data_const(const struct cpymo_engine *e);

void cpymo_list_ui_exit(struct cpymo_engine *e);

void cpymo_list_ui_enable_loop(struct cpymo_engine *);

const void *cpymo_list_ui_get_current_selected_const(const struct cpymo_engine *);

#endif
