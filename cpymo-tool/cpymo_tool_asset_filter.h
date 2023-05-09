#ifndef INCLUDE_CPYMO_TOOL_ASSET_FILTER
#define INCLUDE_CPYMO_TOOL_ASSET_FILTER

#include "../cpymo/cpymo_assetloader.h"
#include "cpymo_tool_asset_analyzer.h"
typedef struct {
    // input
    bool input_is_package;
    union {
        struct {
            cpymo_package *pkg;
            cpymo_package_index file_index;
        } package;

        struct {
            const char *path;
        } file;
    } input;

    // output
    bool output_to_package;
    union {
        struct {
            void **buf;
            size_t *len;
        } package;

        struct {
            const char *target_file_path;
        } file;
    } output;
} cpymo_tool_asset_filter_io;

typedef struct {
    const char *input_gamedir;
    cpymo_assetloader input_assetloader;

    const char *output_gamedir;
    // output type
    //  forced packed/unpacked

    // filter function

    cpymo_tool_asset_analyzer_result result;
} cpymo_tool_asset_filter;

#endif