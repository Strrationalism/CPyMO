#ifndef INCLUDE_CPYMO_BACKEND_MASKTRANS
#define INCLUDE_CPYMO_BACKEND_MASKTRANS

#include "../../cpymo/cpymo_error.h"
#include "../../cpymo/cpymo_color.h"

typedef void *cpymo_backend_text;

error_t cpymo_backend_text_create(cpymo_backend_text *out, const char *utf8_string, float single_character_size_in_logical_screen);
void cpymo_backend_text_free(cpymo_backend_text);

void cpymo_backend_text_draw(cpymo_backend_text, float x, float y, cpymo_color col, float alpha);

#endif
