#include "cpymo_prelude.h"
#include "cpymo_script.h"
#include <string.h>

error_t cpymo_script_load(
    cpymo_script **out, 
    cpymo_str script_name, 
    const cpymo_assetloader *l)
{
    cpymo_script *script = (cpymo_script *)malloc(sizeof(cpymo_script));
    if (script == NULL) return CPYMO_ERR_OUT_OF_MEM;

    script->script_name = cpymo_str_copy_malloc(script_name);
    if (script->script_name == NULL) {
        free(script);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    script->script_content = NULL;
    error_t err = cpymo_assetloader_load_script(
        &script->script_content, 
        &script->script_content_len, 
        script->script_name, 
        l);

    if (err != CPYMO_ERR_SUCC) {
        free(script->script_name);
        free(script);
        return err;
    }

    *out = script;
    return CPYMO_ERR_SUCC;
}

error_t cpymo_script_create_bootloader(cpymo_script **out, char *startscript)
{
    const char *script_format =
		"#textbox message,name\n"
		"#bg logo1\n"
		"#bg logo2\n"
		"#change %s";

    cpymo_script *script = (cpymo_script *)malloc(sizeof(cpymo_script));
    if (script == NULL) return CPYMO_ERR_OUT_OF_MEM;

    script->script_name = cpymo_str_copy_malloc(cpymo_str_pure(""));
    if (script->script_name == NULL) {
        free(script);
        return CPYMO_ERR_OUT_OF_MEM;
    }

	script->script_content = (char *)malloc(53 + strlen(startscript));
    if (script->script_content == NULL) {
        free(script->script_name);
        free(script);
        return CPYMO_ERR_OUT_OF_MEM;
    }

	sprintf(script->script_content, script_format, startscript);
    script->script_content_len = strlen(script->script_content);
    *out = script;
    return CPYMO_ERR_SUCC;
}

void cpymo_script_free(cpymo_script *to_free)
{
    free(to_free->script_content);
    free(to_free->script_name);
    free(to_free);
}

