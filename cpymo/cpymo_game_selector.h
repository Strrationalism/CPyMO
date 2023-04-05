#ifndef INCLUDE_CPYMO_GAME_SELECTOR
#define INCLUDE_CPYMO_GAME_SELECTOR

#include "cpymo_error.h"
#include <stddef.h>
#include "../cpymo-backends/include/cpymo_backend_text.h"
#include "../cpymo-backends/include/cpymo_backend_image.h"

struct cpymo_engine;

typedef struct cpymo_game_selector_item {
	struct cpymo_game_selector_item *next;
	struct cpymo_game_selector_item *prev;
	char *gamedir;
#ifdef ENABLE_TEXT_EXTRACT
	char gametitle_text[256];
#endif

	cpymo_backend_image icon;
	int icon_w, icon_h;

	cpymo_backend_text gametitle;
	float gametitle_w;
} cpymo_game_selector_item;

typedef error_t (*cpymo_game_selector_callback)(struct cpymo_engine *, const char *gamedir);

error_t cpymo_engine_init_with_game_selector(
	struct cpymo_engine *e, size_t screen_w, size_t screen_h, size_t font_size, 
	float empty_message_font_size,
	size_t nodes_per_screen, cpymo_game_selector_item **gamedirs_movein,
	cpymo_game_selector_callback before_reinit,
	cpymo_game_selector_callback after_reinit,
	char **last_selected_game_dir_movein);

error_t cpymo_game_selector_item_create(cpymo_game_selector_item **out, char **game_dir_move_in);
void cpymo_game_selector_item_free_all(cpymo_game_selector_item *item);

#endif
