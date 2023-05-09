#include "cpymo_tool_prelude.h"
#include "cpymo_tool_asset_filter.h"

error_t cpymo_tool_asset_filter_init(
    cpymo_tool_asset_filter *filter,
    const char *input_gamedir,
    const char *output_gamedir);

void cpymo_tool_asset_filter_free(
    cpymo_tool_asset_filter *);

error_t cpymo_tool_asset_filter_run(
    cpymo_tool_asset_filter *);
