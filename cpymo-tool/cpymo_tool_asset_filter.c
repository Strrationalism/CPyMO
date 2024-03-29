#include "cpymo_tool_prelude.h"
#include "../stb/stb_ds.h"
#include "cpymo_tool_image.h"
#include "cpymo_tool_asset_filter.h"
#include "cpymo_tool_asset_analyzer.h"
#include "../stb/stb_ds.h"
#include <time.h>

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

    filter->input_with_mask = cpymo_gameconfig_is_symbian(
        &filter->asset_list.gameconfig);

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
    bool masked, packable;
};


#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#include <direct.h>
#endif

static error_t cpymo_tool_utils_mkdir(const char *gamedir, const char *assdir)
{
    char *full_path = (char *)malloc(strlen(gamedir) + strlen(assdir) + 2);
    if (full_path == NULL) return CPYMO_ERR_OUT_OF_MEM;

    strcpy(full_path, gamedir);
    strcat(full_path, "/");
    strcat(full_path, assdir);

    #ifdef _WIN32
    _mkdir(full_path);
    #else
    mkdir(full_path, 0777);
    #endif

    free(full_path);

    return CPYMO_ERR_SUCC;
}

static error_t cpymo_tool_asset_filter_run_single(
    const cpymo_tool_asset_filter *filter,
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
    if (!param->packable) io.output_to_package = false;

    size_t asset_count = shlenu(param->asset_list);
    if (asset_count) {
        error_t err = cpymo_tool_utils_mkdir(
            filter->output_gamedir, param->asstype);
        CPYMO_THROW(err);
    }
    else {
        return CPYMO_ERR_SUCC;
    }

    cpymo_tool_package_packer packer;
    if (io.output_to_package) {
        char *pack_path = (char *)malloc(
            strlen(filter->output_gamedir)
            + 1
            + 2 * strlen(param->asstype)
            + 6);
        if (pack_path == NULL) return CPYMO_ERR_OUT_OF_MEM;

        strcpy(pack_path, filter->output_gamedir);
        strcat(pack_path, "/");
        strcat(pack_path, param->asstype);
        strcat(pack_path, "/");
        strcat(pack_path, param->asstype);
        strcat(pack_path, ".pak");

        size_t max_package_files = shlenu(param->asset_list);
        if (param->masked && filter->output_with_mask)
            max_package_files *= 2;
        error_t err = cpymo_tool_package_packer_open(
            &packer, pack_path, max_package_files);
        free(pack_path);
        CPYMO_THROW(err);
    }

    time_t log_time = time(NULL);
    for (size_t i = 0; i < asset_count; ++i) {
        // setup input
        io.input_mask_file_buf_movein = NULL;
        io.input_mask_len = 0;

        time_t now_time = time(NULL);
        if (now_time - log_time > 1) {
            log_time = now_time;
            printf("%s %.2f%% %u/%u processed.\n",
                io.asset_type,
                100.0f * (float)i / (float)asset_count,
                (unsigned)i,
                (unsigned)asset_count);
        }

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
        if (io.output_to_package) {
            io.output.package.data_move_out = NULL;
            io.output.package.len = 0;
            io.output.package.mask_move_out = NULL;
            io.output.package.mask_len = 0;
        }
        else {
            io.output.file.asset_name = param->asset_list[i].key;
        }

        io.output_mask_when_symbian = param->asset_list[i].mask;

        // call filter
        error_t err =
            param->filter_function(&io, param->filter_function_userdata);
        if (err != CPYMO_ERR_SUCC) {
            if (param->asset_list[i].warning) {
                printf("[Warning] Can not filter asset: %s/%s(%s).\n",
                    param->asstype,
                    param->asset_list[i].key,
                    cpymo_error_message(err));
            }

            if (io.input_mask_file_buf_movein)
                free(io.input_mask_file_buf_movein);

            if (io.input_is_package)
                if (io.input.package.data_move_in)
                    free(io.input.package.data_move_in);
        }

        // collect output
        if (io.output_to_package) {
            if (io.output.package.data_move_out) {
                err = cpymo_tool_package_packer_add_data(
                    &packer,
                    cpymo_str_pure(param->asset_list[i].key),
                    io.output.package.data_move_out,
                    io.output.package.len);
                if (err != CPYMO_ERR_SUCC) {
                    printf("[Error] Can not write to package: %s/%s(%s).\n",
                        param->asstype,
                        param->asset_list[i].key,
                        cpymo_error_message(err));
                }

                free(io.output.package.data_move_out);
            }

            if (io.output.package.mask_move_out) {
                char *mask_name;
                err = cpymo_tool_get_mask_name_noext(
                    &mask_name, param->asset_list[i].key);
                if (err == CPYMO_ERR_SUCC) {
                    err = cpymo_tool_package_packer_add_data(
                        &packer,
                        cpymo_str_pure(mask_name),
                        io.output.package.mask_move_out,
                        io.output.package.mask_len);
                    free(mask_name);
                }

                if (err != CPYMO_ERR_SUCC) {
                    printf("[Error] Can not write mask to package: %s/%s(%s).\n",
                        param->asstype,
                        param->asset_list[i].key,
                        cpymo_error_message(err));
                }

                free(io.output.package.mask_move_out);
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
          f->input_assetloader.use_pkg_bg ? &f->input_assetloader.pkg_bg : NULL,
          false, true  },

        { "bgm", f->filter_bgm, f->filter_bgm_userdata,
          f->input_assetloader.game_config->bgmformat, NULL, f->asset_list.bgm, NULL,
          false, false },

        { "chara", f->filter_chara, f->filter_chara_userdata,
          f->input_assetloader.game_config->charaformat,
          f->input_assetloader.game_config->charamaskformat,
          f->asset_list.chara,
          f->input_assetloader.use_pkg_chara ? &f->input_assetloader.pkg_chara : NULL,
          true, true },

        { "se", f->filter_se, f->filter_se_userdata,
          f->input_assetloader.game_config->seformat, NULL, f->asset_list.se,
          f->input_assetloader.use_pkg_se ? &f->input_assetloader.pkg_se : NULL,
          false, true },

        { "system", f->filter_system, f->filter_system_userdata,
          "png", "png", f->asset_list.system, NULL,
          true, false },

        { "video", f->filter_video, f->filter_video_userdata,
          "mp4", NULL, f->asset_list.video, NULL,
          false, false },

        { "voice", f->filter_voice, f->filter_voice_userdata,
          f->input_assetloader.game_config->voiceformat, NULL,
          f->asset_list.voice,
          f->input_assetloader.use_pkg_voice ? &f->input_assetloader.pkg_voice : NULL,
          false, true },

        { "script", &cpymo_tool_asset_filter_function_copy, NULL,
          "txt", NULL, f->asset_list.script, NULL,
          false, false }
    };

    error_t err = cpymo_tool_utils_mkdir(f->output_gamedir, "");
    CPYMO_THROW(err);

    for (size_t i = 0; i < CPYMO_ARR_COUNT(params); ++i) {
        err = cpymo_tool_asset_filter_run_single(f, params + i);

        if (err != CPYMO_ERR_SUCC) {
            printf("[Error] Error on processing %s: %s.\n",
                params[i].asstype,
                cpymo_error_message(err));
        }
        else {
            printf("%s OK!\n", params[i].asstype);
        }
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
        io->output.package.data_move_out = data;
        io->output.package.len = len;
        io->output.package.mask_move_out = io->input_mask_file_buf_movein;
        io->output.package.mask_len = io->input_mask_len;
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

error_t cpymo_tool_utils_copy(const char *src_path, const char *dst_path)
{
    char *buf = NULL;
    size_t sz;
    error_t err = cpymo_utils_loadfile(src_path, &buf, &sz);
    CPYMO_THROW(err);

    err = cpymo_tool_utils_writefile(dst_path, buf, sz);
    free(buf);
    CPYMO_THROW(err);

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_utils_copy_gamedir(
    const char *src_gamedir,
    const char *dst_gamedir,
    const char *file_relative_path)
{
    size_t file_relative_path_len = strlen(file_relative_path);
    char *src_path = (char *)malloc(
        strlen(src_gamedir) + file_relative_path_len + 2);

    char *dst_path = (char *)malloc(
        strlen(dst_gamedir) + file_relative_path_len + 2);

    error_t err = CPYMO_ERR_SUCC;

    if (src_path == NULL || dst_path == NULL) {
        err = CPYMO_ERR_OUT_OF_MEM;
        goto END;
    }

    strcpy(src_path, src_gamedir);
    strcpy(dst_path, dst_gamedir);
    strcat(src_path, "/");
    strcat(dst_path, "/");
    strcat(src_path, file_relative_path);
    strcat(dst_path, file_relative_path);

    err = cpymo_tool_utils_copy(src_path, dst_path);

END:
    if (src_path) free(src_path);
    if (dst_path) free(dst_path);
    return err;
}
