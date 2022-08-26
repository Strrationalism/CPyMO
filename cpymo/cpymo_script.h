#ifndef INCLUDE_CPYMO_SCRIPT
#define INCLUDE_CPYMO_SCRIPT

#include "cpymo_error.h"
#include "cpymo_str.h"
#include "cpymo_assetloader.h"

typedef struct {
    char *script_content;
    size_t script_content_len;
    char script_name[];
} cpymo_script;

error_t cpymo_script_load(
    cpymo_script **out, 
    cpymo_str script_name, 
    const cpymo_assetloader *l);

error_t cpymo_script_create_bootloader(
    cpymo_script **out, char *startscript);
    
void cpymo_script_free(cpymo_script *to_free);

#endif
