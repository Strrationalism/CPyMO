#ifndef INCLUDE_CPYMO_TOOL_ASSET_FILTER
#define INCLUDE_CPYMO_TOOL_ASSET_FILTER

#include "../cpymo/cpymo_assetloader.h"
#include "cpymo_tool_asset_analyzer.h"
#include "cpymo_tool_package.h"

typedef struct {
    // input
    bool input_is_package;
    union {
        struct {
            void *data_move_in;
            size_t len;
        } package;

        struct {
            const char *path;
        } file;
    } input;

    void *input_mask_file_buf_movein;
    size_t input_mask_len;

    // output
    bool output_to_package;
    union {
        struct {
            void **data_move_out, **mask_move_out;
            size_t *len, *mask_len;
        } package;

        struct {
            const char *target_file_path;
            const char *target_mask_path;
        } file;
    } output;
} cpymo_tool_asset_filter_io;

typedef error_t (*cpymo_tool_asset_filter_processor)(
    cpymo_tool_asset_filter_io *io,
    void *userdata);

typedef struct {
    // input
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

    // filter function userdata
    void
        *filter_bg_userdata,
        *filter_bgm_userdata,
        *filter_chara_userdata,
        *filter_se_userdata,
        *filter_system_userdata,
        *filter_video_userdata,
        *filter_voice_userdata;

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

error_t cpymo_tool_asset_filter_function_copy(
    cpymo_tool_asset_filter_io *io,
    void *null);

error_t cpymo_tool_utils_writefile(
    const char *path, const void *data, size_t len);

#endif
