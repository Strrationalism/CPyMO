#include "cpymo_tool_prelude.h"
#include "cpymo_tool_asset_filter.h"

static error_t cpymo_tool_strip(
    const char *src_gamedir,
    const char *dst_gamedir)
{
    cpymo_tool_asset_filter filter;
    error_t err = cpymo_tool_asset_filter_init(
        &filter, src_gamedir, dst_gamedir);
    CPYMO_THROW(err);

    filter.filter_bg = &cpymo_tool_asset_filter_function_copy;
    filter.filter_bgm = &cpymo_tool_asset_filter_function_copy;
    filter.filter_chara = &cpymo_tool_asset_filter_function_copy;
    filter.filter_se = &cpymo_tool_asset_filter_function_copy;
    filter.filter_system = &cpymo_tool_asset_filter_function_copy;
    filter.filter_video = &cpymo_tool_asset_filter_function_copy;
    filter.filter_voice = &cpymo_tool_asset_filter_function_copy;

    err = cpymo_tool_asset_filter_run(&filter);
    if (err != CPYMO_ERR_SUCC) {
        cpymo_tool_asset_filter_free(&filter);
        return err;
    }

    cpymo_tool_asset_filter_free(&filter);

    err = cpymo_tool_utils_copy_gamedir(
        src_gamedir, dst_gamedir, "gameconfig.txt");
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] Can not copy gameconfig.txt: %s.\n",
            cpymo_error_message(err));
        return err;
    }

    err = cpymo_tool_utils_copy_gamedir(
        src_gamedir, dst_gamedir, "icon.png");
    if (err != CPYMO_ERR_SUCC) {
        printf("[Warning] Can not copy icon.png: %s.\n",
            cpymo_error_message(err));
    }

    cpymo_tool_utils_copy_gamedir(
        src_gamedir, dst_gamedir, "system/default.ttf");

    printf("=> %s\n", dst_gamedir);

    return CPYMO_ERR_SUCC;
}

int cpymo_tool_invoke_strip(int argc, const char **argv)
{
    extern int help(void);

    if (argc != 4) {
        printf("[Error] Invalid arguments.\n");
        help();
        return -1;
    }

    const char *input = argv[2];
    const char *output = argv[3];
    error_t err = cpymo_tool_strip(input, output);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] %s.\n", cpymo_error_message(err));
        return -1;
    }

    return 0;
}