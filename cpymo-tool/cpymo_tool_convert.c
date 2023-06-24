#include "cpymo_tool_prelude.h"
#include "cpymo_tool_image.h"
#include "cpymo_tool_asset_filter.h"
#include "cpymo_tool_ffmpeg.h"
#include "cpymo_tool_gameconfig.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef uint8_t cpymo_tool_convert_audio_support;

typedef struct {
    const char *name, *desc;

    uint16_t imagesize_w, imagesize_h;
    bool use_mask, play_video, screen_fit_support, forced_audio_convert;
    const char
        *bgformat,
        *charaformat,
        *charamaskformat,
        *platform;
    cpymo_tool_convert_audio_support audio_support;
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

struct cpymo_tool_convert_image_processor_userdata
{
    double scale_ratio;
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
        (size_t)(u->scale_ratio * (double)image.width),
        (size_t)(u->scale_ratio * (double)image.height));
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
            &resized, path, u->target_format, u->mask_format != NULL, u->mask_format);
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
    const char *flags;
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

    #ifdef _WIN32
    #define tempnam _tempnam
    #endif

    if (io->input_is_package) {
        input_file = (char *)malloc(L_tmpnam);
        if (input_file == NULL) return CPYMO_ERR_OUT_OF_MEM;

        tempnam(io->output_gamedir, input_file);
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

        tempnam(io->output_gamedir, output_file);

        delete_output_file = true;
        package_output_file = true;
    }
    else {
        err = cpymo_tool_asset_filter_get_output_file_name(
            &output_file, io, io->output.file.asset_name, u->out_ext);
        delete_output_file = false;
    }

    err = cpymo_tool_ffmpeg_call(
        u->ffmpeg_command, input_file, output_file, u->out_ext, u->flags);

    CLEAN:
    if (input_file) {
        if (delete_input_file)
            remove(input_file);
        free(input_file);
    }

    if (output_file) {
        if (package_output_file) {
            io->output.package.mask_len = 0;
            io->output.package.mask_move_out = NULL;
            io->output.package.data_move_out = NULL;
            io->output.package.len = 0;

            char *buf = NULL; size_t len;
            err = cpymo_utils_loadfile(output_file, &buf, &len);
            if (err == CPYMO_ERR_SUCC) {
                io->output.package.data_move_out = buf;
                io->output.package.len = 0;
            }
        }

        if (delete_output_file)
            remove(output_file);
        free(output_file);
    }

    return err;
}

static error_t cpymo_tool_convert_find_spec(
    const cpymo_tool_convert_spec **out_spec,
    const char *spec_name)
{
    for (size_t i = 0; i < CPYMO_ARR_COUNT(cpymo_tool_convert_specs); ++i) {
        if (!strcmp(spec_name, cpymo_tool_convert_specs[i].name)) {
            *out_spec = cpymo_tool_convert_specs + i;
            return CPYMO_ERR_SUCC;
        }
    }

    printf("[Error] Spec %s not found.\n", spec_name);
    return CPYMO_ERR_NOT_FOUND;
}

static double cpymo_tool_convert_scale(
    size_t src_w, size_t src_h,
    const cpymo_tool_convert_spec *spec)
{
    return (spec->screen_fit_support ? &fmax : &fmin)(
        (double)spec->imagesize_w / (double)src_w,
        (double)spec->imagesize_h / (double)src_h);
}

static bool cpymo_tool_convert_audio_supported(
    cpymo_tool_convert_audio_support support_table,
    cpymo_str format)
{
    #define TEST(FORMAT) \
        ((support_table & FORMAT) && cpymo_str_equals_str_ignore_case(format, #FORMAT))
    return TEST(WAV) || TEST(OGG) || TEST(MP3);
}

static void cpymo_tool_convert_configure_ffmpeg(
    const char *ffmpeg_command,
    const cpymo_tool_convert_spec *spec,
    cpymo_tool_asset_filter_processor *processor,
    void **userdata,
    struct cpymo_tool_convert_ffmpeg_processor_userdata *u,
    const char *current_format)
{
    *processor = &cpymo_tool_asset_filter_function_copy;
    *userdata = NULL;
    u->out_ext = current_format;

    if (!cpymo_tool_convert_audio_supported(
        spec->audio_support,
        cpymo_str_pure(current_format))
        || spec->forced_audio_convert)
    {
        if (ffmpeg_command == NULL) {
            printf("[Warning] ffmpeg not found, audio won\'t be converted.");
            return;
        }

        *processor = &cpymo_tool_convert_ffmpeg_processor;
        *userdata = u;
        u->ffmpeg_command = ffmpeg_command;
        u->flags = NULL;

        if (spec->audio_support & OGG) u->out_ext = "ogg";
        else if (spec->audio_support & MP3) u->out_ext = "mp3";
        else u->out_ext = "wav";
    }
}

static error_t cpymo_tool_convert(
    const char *dst_gamedir,
    const char *src_gamedir,
    const cpymo_tool_convert_spec *spec)
{
    const char *ffmpeg_command;
    error_t err = cpymo_tool_ffmpeg_search(&ffmpeg_command);
    if (err != CPYMO_ERR_SUCC) ffmpeg_command = NULL;

    cpymo_tool_asset_filter filter;
    err = cpymo_tool_asset_filter_init(
        &filter, src_gamedir, dst_gamedir);
    CPYMO_THROW(err);

    cpymo_gameconfig cfg = filter.asset_list.gameconfig;

    filter.filter_bg = &cpymo_tool_convert_image_processor;
    struct cpymo_tool_convert_image_processor_userdata u_bg;
    u_bg.mask_format = NULL;
    u_bg.target_format = spec->bgformat;
    u_bg.scale_ratio = cpymo_tool_convert_scale(
        cfg.imagesize_w, cfg.imagesize_h, spec);
    filter.filter_bg_userdata = &u_bg;

    struct cpymo_tool_convert_ffmpeg_processor_userdata u_bgm;
    cpymo_tool_convert_configure_ffmpeg(
        ffmpeg_command, spec, &filter.filter_bgm,
        &filter.filter_bgm_userdata, &u_bgm, cfg.bgmformat);

    filter.filter_chara = &cpymo_tool_convert_image_processor;
    struct cpymo_tool_convert_image_processor_userdata u_chara = u_bg;
    u_chara.target_format = spec->charaformat;
    if (spec->use_mask) u_chara.mask_format = spec->charamaskformat;
    filter.filter_chara_userdata = &u_chara;

    struct cpymo_tool_convert_ffmpeg_processor_userdata u_se;
    cpymo_tool_convert_configure_ffmpeg(
        ffmpeg_command, spec, &filter.filter_se,
        &filter.filter_se_userdata, &u_se, cfg.seformat);

    filter.filter_system = &cpymo_tool_convert_image_processor;
    struct cpymo_tool_convert_image_processor_userdata u_system = u_bg;
    u_system.mask_format = spec->use_mask ? "png" : NULL;
    u_system.target_format = "png";
    filter.filter_system_userdata = &u_system;

    char u_video_flag_str[256];
    struct cpymo_tool_convert_ffmpeg_processor_userdata u_video;
    if (ffmpeg_command == NULL) {
        printf("[Warning] ffmpeg not found, video won\'t be converted.\n");
        filter.filter_video = &cpymo_tool_asset_filter_function_copy;
        filter.filter_video_userdata = NULL;
    }
    else {
        filter.filter_video = &cpymo_tool_convert_ffmpeg_processor;
        filter.filter_video_userdata = &u_video;
        u_video.ffmpeg_command = ffmpeg_command;
        u_video.out_ext = "mp4";

        char scale_ratio_str[64];
        snprintf(
            scale_ratio_str,
            CPYMO_ARR_COUNT(scale_ratio_str),
            "%lf",
            u_bg.scale_ratio);

        snprintf(
            u_video_flag_str,
            CPYMO_ARR_COUNT(u_video_flag_str),
            "-vf scale=\"iw * %s : ih * %s\" -c:v mpeg4 -c:a aac",
            scale_ratio_str,
            scale_ratio_str);

        u_video.flags = u_video_flag_str;
    }

    struct cpymo_tool_convert_ffmpeg_processor_userdata u_voice;
    cpymo_tool_convert_configure_ffmpeg(
        ffmpeg_command, spec, &filter.filter_voice,
        &filter.filter_voice_userdata, &u_voice, cfg.voiceformat);

    err = cpymo_tool_asset_filter_run(&filter);
    if (err != CPYMO_ERR_SUCC) goto CLEAN;

    #define COPY_STR(DST, SRC) \
        if (SRC != NULL) { \
            for (size_t i = 0; i < CPYMO_ARR_COUNT(DST); ++i) { \
                DST[i] = SRC[i]; \
                if (SRC[i] == '\0') { \
                    for (; i < CPYMO_ARR_COUNT(DST); ++i) \
                        DST[i] = '\0'; \
                    break; \
                } \
            } \
        } \
        else \
            for (size_t i = 0; i < CPYMO_ARR_COUNT(DST); ++i) DST[i]= '\0';

    COPY_STR(cfg.bgformat, u_bg.target_format);
    COPY_STR(cfg.bgmformat, u_bgm.out_ext);
    COPY_STR(cfg.charaformat, u_chara.target_format);
    COPY_STR(cfg.charamaskformat, u_chara.mask_format);
    cfg.imagesize_w = (uint16_t)(cfg.imagesize_w * u_bg.scale_ratio);
    cfg.imagesize_h = (uint16_t)(cfg.imagesize_h * u_bg.scale_ratio);
    COPY_STR(cfg.platform, spec->platform);
    if (!cfg.playvideo) cfg.playvideo = false;
    COPY_STR(cfg.seformat, u_se.out_ext);
    COPY_STR(cfg.voiceformat, u_voice.out_ext);

    char *path = (char *)malloc(strlen(dst_gamedir) + 16);
    if (path == NULL) {
        err = CPYMO_ERR_OUT_OF_MEM;
        goto CLEAN;
    }

    strcpy(path, dst_gamedir);
    strcat(path, "/gameconfig.txt");
    err = cpymo_tool_gameconfig_write_to_file(path, &cfg);
    free(path);
    if (err != CPYMO_ERR_SUCC) goto CLEAN;

    error_t warn = cpymo_tool_utils_copy_gamedir(
        src_gamedir, dst_gamedir, "icon.png");
    if (warn != CPYMO_ERR_SUCC) {
        printf("[Warning] Can not copy icon.png: %s.\n",
            cpymo_error_message(warn));
    }

    cpymo_tool_utils_copy_gamedir(
        src_gamedir, dst_gamedir, "system/default.ttf");

    printf("=> %s\n", dst_gamedir);

    CLEAN:
    cpymo_tool_asset_filter_free(&filter);
    return err;
}

int cpymo_tool_invoke_convert(int argc, const char **argv)
{
    extern int help(void);

    if (argc != 5) {
        printf("[Error] Invalid arguments.\n");
        help();
        return -1;
    }

    const char *spec_name = argv[2];
    const char *input = argv[3];
    const char *output = argv[4];

    const cpymo_tool_convert_spec *spec;
    error_t err = cpymo_tool_convert_find_spec(
        &spec, spec_name);

    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] Can not find spec \'%s\'.\n", spec_name);
        return -1;
    }

    err = cpymo_tool_convert(output, input, spec);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] %s.\n", cpymo_error_message(err));
        return -1;
    }

    return 0;
}
