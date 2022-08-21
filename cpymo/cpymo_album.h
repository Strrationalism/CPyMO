#ifndef INCLUDE_CPYMO_ALBUM
#define INCLUDE_CPYMO_ALBUM

#include "cpymo_error.h"
#include "cpymo_parser.h"

struct cpymo_engine;

uint64_t cpymo_album_cg_name_hash(cpymo_string cg_filename);

error_t cpymo_album_cg_unlock(struct cpymo_engine *, cpymo_string cg_filename);

error_t cpymo_album_enter(
	struct cpymo_engine *e, 
	cpymo_string album_list_name, 
	cpymo_string album_ui_name,
	size_t page);

#endif
