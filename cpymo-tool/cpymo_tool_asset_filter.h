#ifndef INCLUDE_CPYMO_TOOL_ASSET_FILTER
#define INCLUDE_CPYMO_TOOL_ASSET_FILTER

#include "../cpymo/cpymo_assetloader.h"
#include "cpymo_tool_asset_analyzer.h"

typedef struct {
    // input
    bool input_is_package;
    union {
        struct {
            const cpymo_package *pkg;
            const cpymo_package_index file_index;
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
            bool *free_buf_after_write;
        } package;

        struct {
            const char *target_file_path;
        } file;
    } output;
} cpymo_tool_asset_filter_io;

typedef error_t (*cpymo_tool_asset_filter_processor)(
    cpymo_tool_asset_filter_io *io,
    void *userdata);

typedef struct {
    // input
    const char *input_gamedir;
    cpymo_assetloader input_assetloader;

    // output
    const char *output_gamedir;
    bool use_force_pack_unpack_flag;
    bool force_pack_unpack_flag_packed;

    // filter function
    cpymo_tool_asset_filter_processor
        filter_bg,
        filter_bgm,
        filter_chara,
        filter_se,
        filter_system,
        filter_video,
        filter_voice;

    void *filter_userdata;

    cpymo_tool_asset_analyzer_result asset_list;
} cpymo_tool_asset_filter;

error_t cpymo_tool_asset_filter_init(
    cpymo_tool_asset_filter *filter,
    const char *input_gamedir,
    const char *output_gamedir);

void cpymo_tool_asset_filter_free(
    cpymo_tool_asset_filter *);

error_t cpymo_tool_asset_filter_run(
    cpymo_tool_asset_filter *);

#endif
