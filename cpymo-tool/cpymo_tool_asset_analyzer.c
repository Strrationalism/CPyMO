#include "cpymo_tool_prelude.h"
#include "cpymo_tool_asset_analyzer.h"
#include <stdio.h>
#include "../stb/stb_ds.h"
#include  "../cpymo/cpymo_parser.h"

static inline void cpymo_tool_asset_analyzer_free_result_hashset(
    struct cpymo_tool_asset_analyzer_string_hashset_item *r)
{
    size_t len = shlenu(r);
    for (size_t i = 0; i < len; ++i)
        if (r[i].key) free(r[i].key);
    shfree(r);
}

void cpymo_tool_asset_analyzer_free_result(
    cpymo_tool_asset_analyzer_result *r)
{
    cpymo_tool_asset_analyzer_free_result_hashset(r->bg);
    cpymo_tool_asset_analyzer_free_result_hashset(r->bgm);
    cpymo_tool_asset_analyzer_free_result_hashset(r->chara);
    cpymo_tool_asset_analyzer_free_result_hashset(r->script);
    cpymo_tool_asset_analyzer_free_result_hashset(r->se);
    cpymo_tool_asset_analyzer_free_result_hashset(r->system);
    cpymo_tool_asset_analyzer_free_result_hashset(r->video);
    cpymo_tool_asset_analyzer_free_result_hashset(r->voice);
}

typedef struct {
    bool system_sel_highlight;
    bool system_option;
    bool system_cv_thumb;
    bool system_message_cursor;
    bool script_music_list;
} cpymo_tool_asset_analyzer_system_asset_table;

static void cpymo_tool_asset_analyze_single_command(
    const char *script_name,
    const char *gamedir,
    cpymo_str command, 
    cpymo_parser *parser,
    cpymo_tool_asset_analyzer_result *r,
    cpymo_tool_asset_analyzer_system_asset_table *sat);

static error_t cpymo_tool_asset_analyze_single_script(
    const char *gamedir,
    cpymo_tool_asset_analyzer_result *result,
    const char *script_name,
    cpymo_tool_asset_analyzer_system_asset_table *system_asset_table)
{
    if (shgetp_null(result->script, script_name)) return CPYMO_ERR_SUCC;

    char *script_path = malloc(strlen(gamedir) + strlen(script_name) + 13);
    strcpy(script_path, gamedir);
    strcat(script_path, "/script/");
    strcat(script_path, script_name);
    strcat(script_path, ".txt");
    char *script = NULL;
    size_t script_len;
    error_t err = cpymo_utils_loadfile(script_path, &script, &script_len);
    free(script_path);

    struct cpymo_tool_asset_analyzer_string_hashset_item item;
    item.key = cpymo_str_copy_malloc(cpymo_str_pure(script_name));
    item.ext = "txt";
    item.mask = false;
    item.mask_ext = NULL;
    if (item.key == NULL) return CPYMO_ERR_OUT_OF_MEM;
    shputs(result->script, item);

    cpymo_parser parser;
    cpymo_parser_init(&parser, script, script_len);

    do {
        cpymo_str command = cpymo_parser_curline_pop_command(&parser);
        if (cpymo_str_equals_str(command, "")) continue;

        const char *ignore_commands[] = {
            "text_off", "waitkey", "title", "title_dsp",
            "chara_cls", "chara_pos", "flash", "quake", "fade_out", "fade_in",
            "chara_quake", "chara_down", "chara_up", "anime_off", "chara_anime",
            "set", "add", "sub", "label", "goto", "if", "ret",
            "wait", "wait_se", "rand",
            "bgm_stop", "se_stop",
            "load", "config"
        };

        for (size_t i = 0; i < CPYMO_ARR_COUNT(ignore_commands); ++i)
            if (cpymo_str_equals_str(command, ignore_commands[i])) 
                continue;

        if (cpymo_str_equals_str(command, "change") 
            || cpymo_str_equals_str(command, "call"))
        {
            cpymo_str next_script = cpymo_parser_curline_pop_commacell(&parser);
            cpymo_str_trim(&next_script);

            if (cpymo_str_equals_str(next_script, "")) {
                printf("[Error] Script %s(%u): invalid argument.\n",
                    script_name, (unsigned)(parser.cur_line + 1));
                continue;
            }

            char *next_script_name = cpymo_str_copy_malloc(next_script);
            if (next_script_name == NULL) return CPYMO_ERR_OUT_OF_MEM;
            error_t err = cpymo_tool_asset_analyze_single_script(
                gamedir, result, next_script_name, system_asset_table);
            free(next_script_name);

            CPYMO_THROW(err);

            continue;
        }

        cpymo_tool_asset_analyze_single_command(
            script_name, gamedir, command, &parser, result, system_asset_table);

    } while (cpymo_parser_next_line(&parser));

    free(script);

    return CPYMO_ERR_SUCC;
}

#define ADD_ASSET(PRESULT, ASSET_TYPE, ASSET_NAME, EXT, MASKED, MASK_EXT, WARNING) { \
    struct cpymo_tool_asset_analyzer_string_hashset_item item; \
    item.key = cpymo_str_copy_malloc(ASSET_NAME); \
    item.ext = EXT; \
    item.mask = MASKED; \
    item.mask_ext = MASK_EXT; \
    item.warning = WARNING; \
    if (!cpymo_gameconfig_is_symbian(&PRESULT->gameconfig)) { \
        item.mask = false; \
        item.mask_ext = NULL; \
    } \
    if (item.key == NULL) { \
        printf("[Error] Out of memory.\n"); \
        return; \
    } \
    if (shgetp_null(PRESULT->ASSET_TYPE, item.key) == NULL) \
        shputs(PRESULT->ASSET_TYPE, item);\
    else free(item.key); \
}

static void cpymo_tool_asset_analyzer_add_system_assets(
    cpymo_tool_asset_analyzer_result *r,
    const cpymo_tool_asset_analyzer_system_asset_table *t)
{
    ADD_ASSET(r, bg, cpymo_str_pure("logo1"), 
        r->gameconfig.bgformat, false, NULL, true);

    ADD_ASSET(r, bg, cpymo_str_pure("logo2"),
        r->gameconfig.bgformat, false, NULL, true);

    ADD_ASSET(r, system, cpymo_str_pure("menu"), "png", true, "png", true);
    ADD_ASSET(r, system, cpymo_str_pure("message"), "png", true, "png", true);
    ADD_ASSET(r, system, cpymo_str_pure("name"), "png", true, "png", true);

    if (t->system_sel_highlight) {
        ADD_ASSET(r, system, 
            cpymo_str_pure("sel_highlight"), "png", true, "png", true);
    }

    if (t->system_message_cursor) {
        ADD_ASSET(r, system,
            cpymo_str_pure("message_cursor"), "png", true, "png", true);
    }

    if (t->script_music_list) {
        ADD_ASSET(r, script, 
            cpymo_str_pure("music_list"), "txt", false, NULL, true);
    }

    if (t->system_option) {
        ADD_ASSET(r, system, 
            cpymo_str_pure("option"), "png", true, "png", true);
    }

    if (t->system_cv_thumb) {
        ADD_ASSET(r, system, 
            cpymo_str_pure("cvThumb"), "png", false, NULL, true);
    }
}

error_t cpymo_tool_asset_analyze(
    const char *gamedir,
    cpymo_tool_asset_analyzer_result *output)
{
    char *gameconfig_path = malloc(strlen(gamedir) + 16);
    if (gameconfig_path == NULL) return CPYMO_ERR_OUT_OF_MEM;

    strcpy(gameconfig_path, gamedir);
    strcat(gameconfig_path, "/gameconfig.txt");
    
    error_t err = 
        cpymo_gameconfig_parse_from_file(&output->gameconfig, gameconfig_path);
    free(gameconfig_path);
    CPYMO_THROW(err);

    cpymo_pymo_version engineversion = { 1, 2 };
    if (!cpymo_pymo_version_compatible(
        output->gameconfig.engineversion,
        engineversion)) {
        printf("[Error] Game requires engine %d.%d, not compatible with pymo 1.2.",
            output->gameconfig.engineversion.major,
            output->gameconfig.engineversion.minor);
        return CPYMO_ERR_UNSUPPORTED;
    }

    if (cpymo_gameconfig_scripttype_is_pymo(&output->gameconfig)) {
        printf("[Error] Script type %s is not compatible.\n",
            output->gameconfig.scripttype);
        return CPYMO_ERR_UNSUPPORTED;
    }

    bool platform_supported =
        strcmp(output->gameconfig.platform, "pygame") == 0 ||
        strcmp(output->gameconfig.platform, "s60v3") == 0 ||
        strcmp(output->gameconfig.platform, "s60v5") == 0;

    if (!platform_supported) {
        printf("[Error] Platform %s is not supported.\n",
            output->gameconfig.platform);
        return CPYMO_ERR_UNSUPPORTED;
    }

    output->bg = NULL;
    output->bgm = NULL;
    output->chara = NULL;
    output->script = NULL;
    output->se = NULL;
    output->system = NULL;
    output->video = NULL;
    output->voice = NULL;

    cpymo_tool_asset_analyzer_system_asset_table system_asset_table;
    memset(&system_asset_table, 0, sizeof(system_asset_table));

    err = cpymo_tool_asset_analyze_single_script(
        gamedir,
        output,
        output->gameconfig.startscript,
        &system_asset_table);
    cpymo_tool_asset_analyzer_add_system_assets(output, &system_asset_table);
    if (err != CPYMO_ERR_SUCC) 
        cpymo_tool_asset_analyzer_free_result(output);

    return err;
}

static void cpymo_tool_asset_analyze_single_command(
    const char *script_name,
    const char *gamedir,
    cpymo_str cmd, 
    cpymo_parser *parser,
    cpymo_tool_asset_analyzer_result *r,
    cpymo_tool_asset_analyzer_system_asset_table *system_asset_table)
{
    if (cpymo_str_equals_str(cmd, "say")) {
        system_asset_table->system_sel_highlight = true;
        system_asset_table->system_message_cursor = true;
        return;
    }

    if (cpymo_str_equals_str(cmd, "text")) {
        system_asset_table->system_message_cursor = true;
        return;
    }

    if (cpymo_str_equals_str(cmd, "chara")) {
        while (true) {
            cpymo_parser_curline_pop_commacell(parser);
            if (parser->is_line_end) break;
            cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
            cpymo_str_trim(&file);
            cpymo_parser_curline_pop_commacell(parser);
            cpymo_parser_curline_pop_commacell(parser);

            if (!cpymo_str_equals_str(file, "NULL") 
                && !cpymo_str_equals_str(file, "")) {
                ADD_ASSET(r, chara, file, r->gameconfig.charaformat, 
                    true, r->gameconfig.charamaskformat, true);
            }
        }

        return;
    }

    if (cpymo_str_equals_str(cmd, "bg")) {
        cpymo_str bg = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&bg);
        ADD_ASSET(r, bg, bg, r->gameconfig.bgformat, false, NULL, true);
        cpymo_str trans = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&trans);
        if (!cpymo_str_equals_str(trans, "")
            && !cpymo_str_equals_str(trans, "BG_NOFADE")
            && !cpymo_str_equals_str(trans, "BG_ALPHA")
            && !cpymo_str_equals_str(trans, "BG_FADE")) {
            ADD_ASSET(r, system, trans, "png", false, NULL, true);
        }

        return;
    }

    if (cpymo_str_equals_str(cmd, "movie")) {
        cpymo_str m = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&m);
        ADD_ASSET(r, video, m, "mp4", false, NULL, true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "textbox")) {
        cpymo_str a = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&a);
        ADD_ASSET(r, system, a, "png", false, NULL, true);
        a = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&a);
        ADD_ASSET(r, system, a, "png", false, NULL, true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "scroll")) {
        cpymo_str f = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&f);
        ADD_ASSET(r, bg, f, r->gameconfig.bgformat, false, NULL, true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "chara_y")) {
        cpymo_parser_curline_pop_commacell(parser);
        while (true) {
            cpymo_parser_curline_pop_commacell(parser);
            if (parser->is_line_end) break;
            cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
            cpymo_str_trim(&file);

            cpymo_parser_curline_pop_commacell(parser);
            cpymo_parser_curline_pop_commacell(parser);
            cpymo_parser_curline_pop_commacell(parser);

            if (!cpymo_str_equals_str(file, "")) {
                ADD_ASSET(r, chara, file, r->gameconfig.charaformat, 
                    true, r->gameconfig.charamaskformat, true);
            }
        }

        return;
    }

    if (cpymo_str_equals_str(cmd, "chara_scroll")) {
        cpymo_parser_curline_pop_commacell(parser);
        cpymo_parser_curline_pop_commacell(parser);
        cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&file);
        if (!cpymo_str_equals_str(file, "")) {
            ADD_ASSET(r, chara, file, r->gameconfig.charaformat, 
                true, r->gameconfig.charamaskformat, true);
        }

        return;
    }

    if (cpymo_str_equals_str(cmd, "anime_on")) {
        cpymo_parser_curline_pop_commacell(parser);
        cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&file);
        ADD_ASSET(r, system, file, "png", true, "png", true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "sel")) {
        system_asset_table->system_option = true;
        system_asset_table->system_sel_highlight = true;

        cpymo_parser_curline_pop_commacell(parser);
        cpymo_str hint_pic = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&hint_pic);

        if (!cpymo_str_equals_str(hint_pic, "")) {
            char *hint_pic_cstr = (char *)malloc(hint_pic.len + 2);
            if (hint_pic_cstr == NULL) {
                printf("[Error] Out of memory.\n");
                return;
            }

            cpymo_str_copy(hint_pic_cstr, hint_pic.len + 2, hint_pic);
            hint_pic_cstr[hint_pic.len + 1] = '\0';
            for (char i = '0'; i < '4'; ++i) {
                hint_pic_cstr[hint_pic.len] = i;
                ADD_ASSET(r, system, cpymo_str_pure(hint_pic_cstr), 
                    "png", true, "png", true);
            }

            free(hint_pic_cstr);
        }
        
        return;
    }
    

    if (cpymo_str_equals_str(cmd, "select_text")
        || cpymo_str_equals_str(cmd, "select_var")) {
        system_asset_table->system_sel_highlight = true;
        return;
    }

    if (cpymo_str_equals_str(cmd, "select_img")) {
        cpymo_parser_curline_pop_commacell(parser);
        cpymo_str f = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&f);
        ADD_ASSET(r, system, f, "png", true, "png", true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "select_imgs")) {
        cpymo_str choice_num_str = cpymo_parser_curline_pop_commacell(parser);
        int choice_num = cpymo_str_atoi(choice_num_str);
        for (int i = 0; i < choice_num; ++i) {
            cpymo_str f = cpymo_parser_curline_pop_commacell(parser);
            cpymo_str_trim(&f);
            ADD_ASSET(r, system, f, "png", true, "png", true);
            cpymo_parser_curline_pop_commacell(parser);
            cpymo_parser_curline_pop_commacell(parser);
            cpymo_parser_curline_pop_commacell(parser);
        }

        return;
    }

    if (cpymo_str_equals_str(cmd, "bgm")) {
        cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&file);
        ADD_ASSET(r, bgm, file, r->gameconfig.bgmformat, false, NULL, true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "se")) {
        cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&file);
        ADD_ASSET(r, se, file, r->gameconfig.seformat, false, NULL, true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "vo")) {
        cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&file);
        ADD_ASSET(r, voice, file, r->gameconfig.voiceformat, false, NULL, true);
        return;
    }

    if (cpymo_str_equals_str(cmd, "album")) {
        system_asset_table->system_cv_thumb = true;
        
        cpymo_str 
            album_png = cpymo_str_pure("albumbg"),
            album_list = cpymo_str_pure("album_list"),
            arg = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&arg);

        if(!cpymo_str_equals_str(arg, "")) {
            album_png = arg;
            album_list = arg;
        }

        ADD_ASSET(r, script, album_list, "txt", false, NULL, true);
        ADD_ASSET(r, system, album_png, "png", false, NULL, false);

        // other images
        // 1. load album_list
        char *album_list_path = (char *)malloc(
            strlen(gamedir) 
            + strlen("/script/") 
            + album_list.len 
            + strlen(".txt") 
            + 1);

        if (album_list_path == NULL) {
            printf("[Error] Out of memory.\n");
            return;
        }

        strcpy(album_list_path, gamedir);
        strcat(album_list_path, "/script/");
        cpymo_str_copy(album_list_path + strlen(album_list_path), 
            album_list.len + 1, album_list);
        strcat(album_list_path, ".txt");

        char *album_list_buf = NULL;
        size_t album_list_sz;
        error_t err = cpymo_utils_loadfile(
            album_list_path, &album_list_buf, &album_list_sz);
        free(album_list_path);
        if (err != CPYMO_ERR_SUCC) return;

        cpymo_parser album_list_parser;
        cpymo_parser_init(&album_list_parser, album_list_buf, album_list_sz);

        // 2. get max album page id
        int max_page_id = 0;
        do {
            cpymo_str page_ids = cpymo_parser_curline_pop_commacell(
                &album_list_parser);

            int page_id = cpymo_str_atoi(page_ids);
            if (page_id >= max_page_id) max_page_id = page_id;
        } while(cpymo_parser_next_line(&album_list_parser));
        free(album_list_buf);

        // 3. add asset
        char *assname = (char *)malloc(album_png.len + 32);
        if (assname == NULL) {
            printf("[Error] Out of memory.\n");
            return;
        }

        cpymo_str_copy(assname, album_png.len + 1, album_png);
        for (int i = 0; i < max_page_id; ++i) {
            sprintf(assname + album_png.len, "_%d", i);
            ADD_ASSET(r, system, cpymo_str_pure(assname), 
                "png", false, NULL, false);
        }

        free(assname);

        return;
    }

    if (cpymo_str_equals_str(cmd, "music")) {
        system_asset_table->script_music_list = true;
        return;
    }

    if (cpymo_str_equals_str(cmd, "date")) {
        cpymo_str file = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&file);
        if (!cpymo_str_equals_str(cmd, "")) {
            ADD_ASSET(r, system, file, "png", false, NULL, true);
        }
        return;
    }

    printf("[Error] %s(%u): Unknown command: ", 
        script_name, (unsigned)(parser->cur_line + 1));
    for (size_t i = 0; i < cmd.len; ++i)
        putchar(cmd.begin[i]);
    putchar('\n');
}

