#ifndef INCLUDE_CPYMO_TOOL_ASSET_ANALYZER
#define INCLUDE_CPYMO_TOOL_ASSET_ANALYZER

#include "../cpymo/cpymo_error.h"
#include "../cpymo/cpymo_gameconfig.h"

struct cpymo_tool_asset_analyzer_string_hashset_item {
    char *key;
    const char *ext;
    bool mask;
    const char *mask_ext;
    bool warning;
};

typedef struct {
    struct cpymo_tool_asset_analyzer_string_hashset_item
        *bg,
        *bgm,
        *chara,
        *se,
        *script,
        *system,
        *video,
        *voice;

    cpymo_gameconfig gameconfig;
} cpymo_tool_asset_analyzer_result;

error_t cpymo_tool_asset_analyze(
    const char *gamedir,
    cpymo_tool_asset_analyzer_result *output);

void cpymo_tool_asset_analyzer_free_result(
    cpymo_tool_asset_analyzer_result *);


#endif