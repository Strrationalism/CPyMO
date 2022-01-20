#ifndef INCLUDE_CPYMO_BACKEND_SAVE
#define INCLUDE_CPYMO_BACKEND_SAVE

#include <stdio.h>

FILE *cpymo_backend_read_save(const char *gamedir, const char *name);
FILE *cpymo_backend_write_save(const char *gamedir, const char *name);

#endif
