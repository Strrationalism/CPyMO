#ifndef CPYMO_NDS_CACHE_H
#define CPYMO_NDS_CACHE_H
#include <stddef.h>
#include <stdbool.h>
#include "cpymo_str.h"
extern bool nds_cache_enabled;
void nds_cache_path(char*,size_t,const char*,const char*,cpymo_str,const char*);
#endif

void nds_cache_hash_path(char*,size_t,const char*,const char*,const char*,const char*);
