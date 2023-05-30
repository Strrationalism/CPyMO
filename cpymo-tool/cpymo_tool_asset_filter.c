#include "cpymo_tool_prelude.h"
#include "../stb/stb_ds.h"
#include "cpymo_tool_image.h"
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
    io.asset_type = param->asstype;
    io.game_config = &filter->asset_list.gameconfig;
    io.input_asset_ext = param->assext;
    io.input_gamedir = filter->input_assetloader.gamedir;
    io.input_mask_ext = param->maskext;
    io.output_gamedir = filter->output_gamedir;

    if (filter->use_force_pack_unpack_flag)
        io.output_to_package = filter->force_pack_unpack_flag_packed;

    size_t asset_count = shlenu(param->asset_list);
    if (asset_count) {
        error_t err = cpymo_tool_utils_mkdir(
            filter->output_gamedir, param->asstype);
        CPYMO_THROW(err);
    }

    cpymo_tool_package_packer packer;
    if (io.output_to_package) {
        char *pack_path = (char *)malloc(
            strlen(filter->output_gamedir)
            + 1
            + 2 * strlen(param->asstype)
            + 6);
        if (pack_path == NULL) return CPYMO_ERR_OUT_OF_MEM;

        error_t err = cpymo_tool_package_packer_open(
            &packer, pack_path, shlenu(param->asset_list));
        CPYMO_THROW(err);
    }

    for (size_t i = 0; i < asset_count; ++i) {
        // setup input
        io.input_mask_file_buf_movein = NULL;
        io.input_mask_len = 0;

        if (io.input_is_package) {
            io.input.package.data_move_in = NULL;
            error_t err = cpymo_package_read_file(
                (char **)&io.input.package.data_move_in,
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
                char *mask_name = (char *)malloc(
                    strlen(param->asset_list[i].key) + 6);
                if (mask_name == NULL) {
                    printf("[Warning] Out of memory.\n");
                    goto BREAK_SETUP_INPUT;
                }

                strcpy(mask_name, param->asset_list[i].key);
                strcat(mask_name, "_mask");

                error_t err = cpymo_package_read_file(
                    (char **)&io.input_mask_file_buf_movein,
                    &io.input_mask_len,
                    param->package,
                    cpymo_str_pure(mask_name));
                if (err != CPYMO_ERR_SUCC) {
                    printf("[Warning] Can not load mask from %s/%s.\n",
                        param->asstype,
                        mask_name);
                    io.input_mask_file_buf_movein = NULL;
                    io.input_mask_len = 0;
                }

                free(mask_name);
            }
        }
        else {
            io.input.file.asset_name = param->asset_list[i].key;
            if (param->asset_list[i].mask) {
                char *mask_path = (char *)malloc(
                    strlen(filter->input_assetloader.gamedir)
                    + 1
                    + strlen(param->asstype)
                    + 1
                    + strlen(io.input.file.asset_name)
                    + 6
                    + strlen(param->maskext)
                    + 1);
                if (mask_path == NULL) {
                    printf("[Warning] Out of memory.\n");
                    goto BREAK_SETUP_INPUT;
                }

                strcpy(mask_path, filter->input_assetloader.gamedir);
                strcat(mask_path, "/");
                strcat(mask_path, param->asstype);
                strcat(mask_path, "/");
                strcat(mask_path, io.input.file.asset_name);
                strcat(mask_path, "_mask.");
                strcat(mask_path, param->maskext); 
                
                error_t err = cpymo_utils_loadfile(
                    mask_path, 
                    (char **)&io.input_mask_file_buf_movein,
                    &io.input_mask_len);
                free(mask_path);
                if (err != CPYMO_ERR_SUCC) {
                    printf("[Warning] Can not load mask for %s/%s.\n",
                        param->asstype, io.input.file.asset_name);
                    io.input_mask_file_buf_movein = NULL;
                    io.input_mask_len = 0;
                    goto BREAK_SETUP_INPUT;
                }
            }
        }

        BREAK_SETUP_INPUT:

        // setup output
        void *write_to_package = NULL, *write_to_package_mask = NULL;
        size_t write_to_package_len = 0, write_to_package_mask_len = 0;
        if (io.output_to_package) {
            io.output.package.data_move_out = &write_to_package;
            io.output.package.len = &write_to_package_len;
            io.output.package.mask_move_out = &write_to_package;
            io.output.package.mask_len = &write_to_package_mask_len;
        }
        else {
            io.output.file.asset_name = param->asset_list[i].key;
        }

        // call filter
        error_t err = 
            param->filter_function(&io, param->filter_function_userdata);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Warning] Can not filter asset: %s/%s(%s).",
                param->asstype,
                param->asset_list[i].key,
                cpymo_error_message(err));

            if (io.input_mask_file_buf_movein)
                free(io.input_mask_file_buf_movein);
                
            if (io.input_is_package)
                if (io.input.package.data_move_in)
                    free(io.input.package.data_move_in);
        }

        // collect output
        if (io.output_to_package) {
            if (write_to_package) {
                err = cpymo_tool_package_packer_add_data(
                    &packer,
                    cpymo_str_pure(param->asset_list[i].key),
                    write_to_package,
                    write_to_package_len);
                if (err != CPYMO_ERR_SUCC) {
                    printf("[Error] Can not write to package: %s/%s(%s).\n",
                        param->asstype,
                        param->asset_list[i].key,
                        cpymo_error_message(err));
                }

                free(write_to_package);
            }

            if (write_to_package_mask) {
                char *mask_name;
                err = cpymo_tool_get_mask_name_noext(
                    &mask_name, param->asset_list[i].key);
                if (err == CPYMO_ERR_SUCC) {
                    err = cpymo_tool_package_packer_add_data(
                        &packer,
                        cpymo_str_pure(mask_name),
                        write_to_package_mask,
                        write_to_package_mask_len);
                    free(mask_name);
                }

                if (err != CPYMO_ERR_SUCC) {
                    printf("[Error] Can not write mask to package: %s/%s(%s).\n",
                        param->asstype,
                        param->asset_list[i].key,
                        cpymo_error_message(err));
                }

                free(write_to_package_mask);
            }
        }
    }

    if (io.output_to_package) {
        cpymo_tool_package_packer_close(&packer);
    }

    return CPYMO_ERR_SUCC;
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
    const cpymo_tool_asset_filter_io *io,
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
        char *path = NULL;
        error_t err = cpymo_tool_asset_filter_get_input_file_name(&path, io);
        CPYMO_THROW(err);
        err = cpymo_utils_loadfile(
            path, (char **)&data, &len);
        free(path);
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
        char *out_path = NULL;
        error_t err = cpymo_tool_asset_filter_get_output_file_name(
            &out_path, io, io->output.file.asset_name, io->input_asset_ext);
        CPYMO_THROW(err);

        err = cpymo_tool_utils_writefile(out_path, data, len);
        free(out_path);
        CPYMO_THROW(err);

        free(data);

        if (io->input_mask_file_buf_movein && io->input_mask_ext) {
            char *mask_name = NULL;
            err = cpymo_tool_get_mask_name_noext(
                &mask_name, io->output.file.asset_name);
            if (err != CPYMO_ERR_SUCC) {
                printf("[Warning] Can not get mask name: %s/%s(%s).\n",
                    io->asset_type,
                    io->output.file.asset_name,
                    cpymo_error_message(err));
                goto MASK_WRITE_FAIL;
            }

            err = cpymo_tool_asset_filter_get_output_file_name(
                &out_path, io, mask_name,
                io->input_mask_ext);
            free(mask_name);
            if (err != CPYMO_ERR_SUCC) {
                printf("[Warning] Can not get mask path: %s(%s)\n",
                    io->output.file.asset_name,
                    cpymo_error_message(err));
                goto MASK_WRITE_FAIL;
            }

            err = cpymo_tool_utils_writefile(
                out_path,
                io->input_mask_file_buf_movein,
                io->input_mask_len);
            free(out_path);
            if (err != CPYMO_ERR_SUCC)
                printf("[Warning] Can not write mask file: %s(%s)",
                    io->output.file.asset_name,
                    cpymo_error_message(err));
        }

        MASK_WRITE_FAIL:

        if (io->input_mask_file_buf_movein)
            free(io->input_mask_file_buf_movein);
    }

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_asset_filter_get_input_file_name(
    char **out, const cpymo_tool_asset_filter_io *io)
{
    *out = (char *)malloc(
        strlen(io->input_gamedir)
        + 1
        + strlen(io->asset_type)
        + 1
        + strlen(io->input.file.asset_name)
        + 1
        + strlen(io->input_asset_ext)
        + 1);
    if (*out == NULL) return CPYMO_ERR_OUT_OF_MEM;

    strcpy(*out, io->input_gamedir);
    strcat(*out, "/");
    strcat(*out, io->asset_type);
    strcat(*out, "/");
    strcat(*out, io->input.file.asset_name);
    strcat(*out, ".");
    strcat(*out, io->input_asset_ext);        
    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_asset_filter_get_output_file_name(
    char **out,
    const cpymo_tool_asset_filter_io *io,
    const char *asset_name,
    const char *ext)
{
    *out = (char *)malloc(
        strlen(io->output_gamedir)
        + 1
        + strlen(io->asset_type)
        + 1
        + strlen(asset_name)
        + 1
        + strlen(ext)
        + 1);
    if (*out == NULL) return CPYMO_ERR_OUT_OF_MEM;

    strcpy(*out, io->output_gamedir);
    strcat(*out, "/");
    strcat(*out, io->asset_type);
    strcat(*out, "/");
    strcat(*out, asset_name);
    strcat(*out, ".");
    strcat(*out, ext);
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
