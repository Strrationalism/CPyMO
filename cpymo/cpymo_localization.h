#ifndef INCLUDE_CPYMO_LOCALIZATION
#define INCLUDE_CPYMO_LOCALIZATION

#include "cpymo_error.h"
#include "cpymo_gameconfig.h"

typedef struct {
	const char
		*msgbox_ok,
		*msgbox_cancel,

		*rmenu_save,
		*rmenu_load,
		*rmenu_skip,
		*rmenu_hide_window,
		*rmenu_backlog,
		*rmenu_config,
		*rmenu_restart,
		*rmenu_back_to_game,

		*rmenu_restat_game_confirm_message,

		*config_bgmvol,
		*config_sevol,
		*config_vovol,
		*config_sayspeed,
		*config_fontsize,

		*config_sayspeeds[6];

	error_t (*save_already_save_to)(char **out_str, int save_id);
	error_t (*save_failed)(char **out_str, error_t);
	error_t (*save_are_you_sure_save_to)(char **out_str, int save_id);
	error_t (*save_title)(char **out_str, int, const char *title);
	error_t (*save_auto_title)(char **out_str, const char *title);
	error_t (*save_are_you_sure_load)(char **out_str, int save_id);
	const char *save_are_you_sure_load_auto_save;

	error_t (*date_str)(char **out_str, int month, int day);

	const char 
		*game_selector_empty,
		*game_selector_empty_secondary;

	const char *mo2pymo_required;
	const char *visual_help_selection;
	error_t (*pymo_version_not_compatible_message)(char **, cpymo_pymo_version);
	const char *exit_confirm;
} cpymo_localization;

struct cpymo_engine;
const cpymo_localization *cpymo_localization_get(struct cpymo_engine *);

#endif
