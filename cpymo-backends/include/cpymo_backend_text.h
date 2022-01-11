#ifndef INCLUDE_CPYMO_BACKEND_TEXT
#define INCLUDE_CPYMO_BACKEND_TEXT

#include "../../cpymo/cpymo_error.h"
#include "../../cpymo/cpymo_color.h"
#include "cpymo_backend_image.h"

typedef void *cpymo_backend_text;

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    const char *utf8_string, 
    float single_character_size_in_logical_screen);

void cpymo_backend_text_free(cpymo_backend_text);

void cpymo_backend_text_draw(
    cpymo_backend_text,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type);

#endif
