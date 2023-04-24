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
            if (cpymo_str_equals_str(command, ignore_commands[i])) continue;

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
            command, &parser, result, system_asset_table);
    } while (cpymo_parser_next_line(&parser));


    free(script);

    return CPYMO_ERR_SUCC;
}

#define ADD_ASSET(PRESULT, ASSET_TYPE, ASSET_NAME, EXT, MASKED, MASK_EXT) { \
    struct cpymo_tool_asset_analyzer_string_hashset_item item; \
    item.key = cpymo_str_copy_malloc(ASSET_NAME); \
    item.ext = EXT; \
    item.mask = MASKED; \
    item.mask_ext = MASK_EXT; \
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
        r->gameconfig.bgformat, false, NULL);

    ADD_ASSET(r, bg, cpymo_str_pure("logo2"),
        r->gameconfig.bgformat, false, NULL);

    ADD_ASSET(r, system, cpymo_str_pure("menu"), "png", true, "png");
    ADD_ASSET(r, system, cpymo_str_pure("message"), "png", true, "png");
    ADD_ASSET(r, system, cpymo_str_pure("name"), "png", true, "png");

    if (t->system_sel_highlight) {
        ADD_ASSET(r, system, 
            cpymo_str_pure("sel_highlight"), "png", true, "png");
    }

    if (t->system_message_cursor) {
        ADD_ASSET(r, system,
            cpymo_str_pure("message_cursor"), "png", true, "png");
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
                    true, r->gameconfig.charamaskformat);
            }
        }

        return;
    }

    if (cpymo_str_equals_str(cmd, "bg")) {
        cpymo_str bg = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&bg);
        ADD_ASSET(r, bg, bg, r->gameconfig.bgformat, false, NULL);
        cpymo_str trans = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&trans);
        if (!cpymo_str_equals_str(trans, "")
            && !cpymo_str_equals_str(trans, "BG_NOFADE")
            && !cpymo_str_equals_str(trans, "BG_ALPHA")
            && !cpymo_str_equals_str(trans, "BG_FADE")) {
            ADD_ASSET(r, system, trans, "png", false, NULL);
        }

        return;
    }

    if (cpymo_str_equals_str(cmd, "movie")) {
        cpymo_str m = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&m);
        ADD_ASSET(r, video, m, "mp4", false, NULL);
        return;
    }

    if (cpymo_str_equals_str(cmd, "textbox")) {
        cpymo_str a = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&a);
        ADD_ASSET(r, system, a, "png", false, NULL);
        a = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&a);
        ADD_ASSET(r, system, a, "png", false, NULL);
        return;
    }

    if (cpymo_str_equals_str(cmd, "scroll")) {
        cpymo_str f = cpymo_parser_curline_pop_commacell(parser);
        cpymo_str_trim(&f);
        ADD_ASSET(r, bg, f, r->gameconfig.bgformat, false, NULL);
        return;
    }

    //chara_y
    //chara_scroll
    //anime_on
    //sel
    //"select_text"
    //"select_var",
    //select_img
    //select_imgs
    //bgm
    //se
    //vo
    //album
    //music
    //date
}


int main()
{
    cpymo_tool_asset_analyzer_result r;
    error_t err = cpymo_tool_asset_analyze("C:/Users/85397/AppData/Roaming/Citra/sdmc/pymogames/DAICHYAN_android", &r);
    if (err != CPYMO_ERR_SUCC) printf("%s\n", cpymo_error_message(err));
    int len = shlen(r.system);
    for (int i = 0; i < len; ++i) {
        if (r.system == NULL) continue;
        printf("%s\n", r.system[i].key);
    }

    cpymo_tool_asset_analyzer_free_result(&r);

    #ifdef LEAKCHECK
	stb_leakcheck_dumpmem();
	#endif	
    return 0;   
}
