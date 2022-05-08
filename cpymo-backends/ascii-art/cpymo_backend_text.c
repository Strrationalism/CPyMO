#include <cpymo_backend_text.h>
#include <cpymo_backend_masktrans.h>

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_parser_stream_span utf8_string, 
    float single_character_size_in_logical_screen)
{
    *out = 0x100;
    *out_width = 64.0f;
    return CPYMO_ERR_SUCC;   
}

void cpymo_backend_text_free(cpymo_backend_text t){}

void cpymo_backend_text_draw(
    cpymo_backend_text t,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type){}

float cpymo_backend_text_width(
    cpymo_parser_stream_span s,
    float single_character_size_in_logical_screen){ return 64.0; }

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
    return CPYMO_ERR_SUCC;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m){}

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans m, float t, bool is_fade_in){}
