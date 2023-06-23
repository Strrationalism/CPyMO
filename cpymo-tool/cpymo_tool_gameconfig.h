#ifndef INCLUDE_CPYMO_TOOL_GAMECONFIG

#include "../cpymo/cpymo_error.h"
#include "../cpymo/cpymo_gameconfig.h"

error_t cpymo_tool_gameconfig_write_to_file(
    const char *path,
    const cpymo_gameconfig *gameconfig);

#endif
