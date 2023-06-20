#include "cpymo_tool_prelude.h"
#include "cpymo_tool_image.h"
#include "cpymo_tool_asset_filter.h"
#include "cpymo_tool_ffmpeg.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    const char *name, *desc;

    uint16_t imagesize_w, imagesize_h;
    bool use_mask, play_video, screen_fit_support, forced_audio_convert;
    const char
        *bgformat,
        *charaformat,
        *charamaskformat,
        *platform;
    uint8_t audio_support;
} cpymo_tool_convert_spec;

#define OGG 0x01
#define MP3 0x02
#define WAV 0x04

static const cpymo_tool_convert_spec cpymo_tool_convert_specs[] = {
    { "s60v3", "PyMO for Symbian S60v3", 320, 240,
      true, true, false, true,
      "jpg", "jpg", "jpg", "s60v3",
      OGG | MP3 | WAV },

    { "s60v5", "PyMO for Symbian S60v5", 540, 360,
      true, true, false, true,
      "jpg", "jpg", "jpg", "s60v5",
      OGG | MP3 | WAV },

    { "3ds", "CPyMO for Nintendo 3DS", 400, 240,
      false, true, true, false,
      "jpg", "png", NULL, "pygame",
      OGG | MP3 | WAV },

    { "pymo", "PyMO", 800, 600,
      false, true, false, false,
      "jpg", "png", NULL, "pygame",
      OGG | WAV },

    { "psp", "CPyMO for Sony Playstation Portable", 480, 272,
      false, false, false, true,
      "jpg", "png", NULL, "pygame",
      OGG | MP3 | WAV },

    { "wii", "CPyMO for Nintendo Wii", 640, 480,
      false, false, false, true,
      "jpg", "png", NULL, "pygame",
      OGG | WAV },
};

#undef OGG
#undef MP3
#undef WAV

struct cpymo_tool_convert_image_processor_userdata
{
    float scale_ratio;
    const char *target_format;
    const char *mask_format;
};

static error_t cpymo_tool_convert_image_processor(
    cpymo_tool_asset_filter_io *io,
    void *userdata)
{
    // load
    cpymo_tool_image image;

    error_t err;
    if (io->input_is_package) {
        err = cpymo_tool_image_load_from_memory(
            &image, io->input.package.data_move_in,
            io->input.package.len, false);
    }
    else {
        char *path;
        err = cpymo_tool_asset_filter_get_input_file_name(&path, io);
        CPYMO_THROW(err);

        err = cpymo_tool_image_load_from_file(
            &image, path, false, NULL);
        free(path);
    }

    CPYMO_THROW(err);

    if (io->input_mask_file_buf_movein) {
        err = cpymo_tool_image_load_attach_mask_from_memory(
            &image, io->input_mask_file_buf_movein, io->input_mask_len);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Warning] Failed to attach mask for %s:%s\n",
                io->asset_type,
                cpymo_error_message(err));
            err = CPYMO_ERR_SUCC;
        }
    }

    // resize
    const struct cpymo_tool_convert_image_processor_userdata *u =
        (struct cpymo_tool_convert_image_processor_userdata*)userdata;

    cpymo_tool_image resized;
    err = cpymo_tool_image_resize(&resized, &image,
        (size_t)(u->scale_ratio * (float)image.width),
        (size_t)(u->scale_ratio * (float)image.height));
    cpymo_tool_image_free(image);

    // write
    if (io->output_to_package) {
        void *data = NULL;
        size_t len;
        err = cpymo_tool_image_save_to_memory(
            resized,
            u->target_format,
            &data,
            &len);
        if (err != CPYMO_ERR_SUCC) goto CLEAN;

        io->output.package.data_move_out = data;
        io->output.package.len = len;

        if (u->mask_format) {
            cpymo_tool_image mask;
            err = cpymo_tool_image_detach_mask(&resized, &mask);
            if (err == CPYMO_ERR_SUCC) {
                void *data = NULL;
                size_t len;
                err = cpymo_tool_image_save_to_memory(
                    mask,
                    u->mask_format,
                    &data,
                    &len);
                cpymo_tool_image_free(mask);

                io->output.package.mask_move_out = data;
                io->output.package.mask_len = len;

                if (err != CPYMO_ERR_SUCC) {
                    io->output.package.mask_move_out = NULL;
                    io->output.package.mask_len = 0;
                }
            }
        }
    }
    else {
        char *path = NULL;
        err = cpymo_tool_asset_filter_get_output_file_name(
            &path, io, io->output.file.asset_name, u->target_format);
        if (err != CPYMO_ERR_SUCC) goto CLEAN;

        err = cpymo_tool_image_save_to_file_with_mask(
            &resized, path, u->target_format, u->target_format != NULL, u->target_format);
        free(path);
    }

    // clean
    CLEAN:
    cpymo_tool_image_free(resized);

    if (io->input_is_package) {
        if (io->input.package.data_move_in)
            free(io->input.package.data_move_in);
    }

    if (io->input_mask_file_buf_movein)
        free(io->input_mask_file_buf_movein);

    return err;
}

struct cpymo_tool_convert_ffmpeg_processor_userdata
{
    const char *ffmpeg_command;
    const char *out_ext;
};

static error_t cpymo_tool_convert_ffmpeg_processor(
    cpymo_tool_asset_filter_io *io,
    void *userdata)
{
    bool delete_input_file = false;
    char *input_file = NULL;
    error_t err = CPYMO_ERR_SUCC;
    char *output_file = NULL;
    bool delete_output_file = false;
    bool package_output_file = false;
    struct cpymo_tool_convert_ffmpeg_processor_userdata *u =
        (struct cpymo_tool_convert_ffmpeg_processor_userdata *)userdata;

    if (io->input_is_package) {
        input_file = (char *)malloc(L_tmpnam);
        if (input_file == NULL) return CPYMO_ERR_OUT_OF_MEM;

        tempnam(input_file);
        err = cpymo_tool_utils_writefile(
            input_file, io->input.package.data_move_in, io->input.package.len);
        if (err != CPYMO_ERR_SUCC) goto CLEAN;
    }
    else {
        err = cpymo_tool_asset_filter_get_input_file_name(
            &input_file, io);
        if (err != CPYMO_ERR_SUCC) goto CLEAN;
    }

    if (io->output_to_package) {
        output_file = (char *)malloc(L_tmpnam);
        if (output_file == NULL) {
            err = CPYMO_ERR_OUT_OF_MEM;
            goto CLEAN;
        }

        tempnam(output_file);

        delete_output_file = true;
        package_output_file = true;
    }
    else {
        err = cpymo_tool_asset_filter_get_output_file_name(
            &output_file, io, io->output.file.asset_name, u->out_ext );
        delete_output_file = false;
    }

    err = cpymo_tool_ffmpeg_call(
        u->ffmpeg_command, input_file, output_file, u->out_ext);

    CLEAN:
    if (input_file) free(input_file);
    if (delete_input_file) { /* TODO */}

    if (output_file) free(output_file);
    if (package_output_file) { /* TODO */}
    if (delete_output_file) { /* TODO */ }
    return CPYMO_ERR_SUCC;
}
