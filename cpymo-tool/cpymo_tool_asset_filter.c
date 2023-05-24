#include "cpymo_tool_prelude.h"
#include "cpymo_tool_asset_filter.h"
#include "cpymo_tool_asset_analyzer.h"
#include "../stb/stb_ds.h"

error_t cpymo_tool_asset_filter_init(
    cpymo_tool_asset_filter *filter,
    const char *input_gamedir,
    const char *output_gamedir)
{
    memset(filter, 0, sizeof(*filter));
    filter->output_gamedir = output_gamedir;

    error_t err = cpymo_tool_asset_analyze(input_gamedir, &filter->asset_list);
    CPYMO_THROW(err);

    err = cpymo_assetloader_init(
        &filter->input_assetloader,
        &filter->asset_list.gameconfig,
        input_gamedir);
    if (err != CPYMO_ERR_SUCC) {
        cpymo_tool_asset_analyzer_free_result(&filter->asset_list);
        return err;
    }

    return CPYMO_ERR_SUCC;
}

void cpymo_tool_asset_filter_free(
    cpymo_tool_asset_filter *f)
{
    cpymo_assetloader_free(&f->input_assetloader);
    cpymo_tool_asset_analyzer_free_result(&f->asset_list);
}

struct cpymo_tool_asset_filter_run_param
{
    const char *asstype;
    cpymo_tool_asset_filter_processor filter_function;
    void *filter_function_userdata;
    const char *assext;
    const char *maskext;
    struct cpymo_tool_asset_analyzer_string_hashset_item *asset_list;
    const cpymo_package *package;
};

static error_t cpymo_tool_utils_mkdir(const char *gamedir, const char *assdir)
{
    return CPYMO_ERR_UNKNOWN;
}

static error_t cpymo_tool_asset_filter_run_single(
    cpymo_tool_asset_filter *filter,
    const struct cpymo_tool_asset_filter_run_param *param)
{
    cpymo_tool_asset_filter_io io;
    memset(&io, 0, sizeof(io));

    io.input_is_package = param->package != NULL;
    io.output_to_package = io.input_is_package;

    if (filter->use_force_pack_unpack_flag)
        io.output_to_package = filter->force_pack_unpack_flag_packed;

    size_t asset_count = shlenu(param->asset_list);
    if (asset_count) {
        error_t err = cpymo_tool_utils_mkdir(
            filter->output_gamedir, param->asstype);
        CPYMO_THROW(err);
    }

    for (size_t i = 0; i < asset_count; ++i) {
        // setup input
        io.input_mask_file_buf_movein = NULL;
        io.input_mask_len = NULL;

        if (io.input_is_package) {
            io.input.package.data_move_in = NULL;
            error_t err = cpymo_package_read_file(
                &io.input.package.data_move_in,
                &io.input.package.len,
                param->package,
                cpymo_str_pure(param->asset_list[i].key));
            if (param->asset_list[i].warning && err != CPYMO_ERR_SUCC) {
                printf("[Warning] Can not read file: %s(%s).\n",
                    param->asset_list[i].key,
                    cpymo_error_message(err));
                continue;
            }

            if (param->asset_list[i].mask) {
                // get mask filename
            }
        }
        // setup output
        // call filter
        // collect output
    }
    return CPYMO_ERR_UNSUPPORTED;
}

error_t cpymo_tool_asset_filter_run(
    cpymo_tool_asset_filter *f)
{
    struct cpymo_tool_asset_filter_run_param params[] = {
        { "bg", f->filter_bg, f->filter_bg_userdata,
          f->input_assetloader.game_config->bgformat, NULL, f->asset_list.bg,
          f->input_assetloader.use_pkg_bg ? &f->input_assetloader.pkg_bg : NULL },

        { "bgm", f->filter_bgm, f->filter_bgm_userdata,
          f->input_assetloader.game_config->bgmformat, NULL, f->asset_list.bgm, NULL },

        { "chara", f->filter_chara, f->filter_chara_userdata,
          f->input_assetloader.game_config->charaformat,
          f->input_assetloader.game_config->charamaskformat,
          f->asset_list.chara,
          f->input_assetloader.use_pkg_chara ? &f->input_assetloader.pkg_chara : NULL },

        { "se", f->filter_se, f->filter_se_userdata,
          f->input_assetloader.game_config->seformat, NULL, f->asset_list.se,
          f->input_assetloader.use_pkg_se ? &f->input_assetloader.pkg_se : NULL },

        { "system", f->filter_system, f->filter_system_userdata,
          "png", "png", f->asset_list.system, NULL },

        { "video", f->filter_video, f->filter_video_userdata,
          "mp4", NULL, f->asset_list.video, NULL },

        { "voice", f->filter_voice, f->filter_voice_userdata,
          f->input_assetloader.game_config->voiceformat, NULL,
          f->asset_list.voice,
          f->input_assetloader.use_pkg_voice ? &f->input_assetloader.pkg_voice : NULL },

        { "script", &cpymo_tool_asset_filter_function_copy, NULL,
          "txt", NULL, f->asset_list.script, NULL }
    };

    for (size_t i = 0; i < CPYMO_ARR_COUNT(params); ++i) {
        printf("Processing %s...", params[i].asstype);
        error_t err = cpymo_tool_asset_filter_run_single(f, params + i);
        CPYMO_THROW(err);
    }

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_asset_filter_function_copy(
    cpymo_tool_asset_filter_io *io,
    void *null)
{
    // input
    void *data = NULL;
    size_t len;

    if (io->input_is_package) {
        data = io->input.package.data_move_in;
        len = io->input.package.len;
    }
    else {
        error_t err = cpymo_utils_loadfile(
            io->input.file.path, (char **)&data, &len);
        CPYMO_THROW(err);
    }

    // output
    if (io->output_to_package) {
        *io->output.package.data_move_out = data;
        *io->output.package.len = len;
        *io->output.package.mask_move_out = io->input_mask_file_buf_movein;
        *io->output.package.mask_len = io->input_mask_len;
    }
    else {
        error_t err = cpymo_tool_utils_writefile(
            io->output.file.target_file_path, data, len);

        if (io->output.file.target_mask_path && io->input_mask_file_buf_movein){
            error_t errm = cpymo_tool_utils_writefile(
                io->output.file.target_mask_path,
                io->input_mask_file_buf_movein,
                io->input_mask_len);
            if (errm != CPYMO_ERR_SUCC)
                printf("[Warning] Can not write mask file: %s(%s)",
                    io->output.file.target_mask_path,
                    cpymo_error_message(errm));
        }

        free(data);
        if (io->input_mask_file_buf_movein)
            free(io->input_mask_file_buf_movein);

        CPYMO_THROW(err);
    }

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_utils_writefile(
    const char *path, const void *data, size_t len)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

    if (fwrite(data, len, 1, file) != 1) {
        fclose(file);
        return CPYMO_ERR_BAD_FILE_FORMAT;
    }

    if (fclose(file)) return CPYMO_ERR_BAD_FILE_FORMAT;
    return CPYMO_ERR_SUCC;
}
