#include <cpymo_backend_text.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include <cpymo_parser.h>

static C2D_Font font;
static C2D_TextBuf for_measure;

error_t cpymo_backend_text_sys_init()
{
    Result res = romfsInit();
    if(R_FAILED(res)) {
        return CPYMO_ERR_UNKNOWN;
    }

    res = cfguInit();
    if(R_FAILED(res)) {
        romfsExit();
        return CPYMO_ERR_UNKNOWN;
    }

    font = C2D_FontLoad("romfs:/font.bcfnt");
    if(font == NULL) {
        cfguExit();
        romfsExit();
        return CPYMO_ERR_UNKNOWN;
    }

    for_measure = C2D_TextBufNew(4096);
    if(for_measure == NULL) {
        C2D_FontFree(font);
        cfguExit();
        romfsExit();
        return CPYMO_ERR_UNKNOWN;
    }

    printf("[Info] Font Loaded!!\n");

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_sys_free()
{
    C2D_TextBufDelete(for_measure);
    C2D_FontFree(font);
    cfguExit();
    romfsExit();
}

struct cpymo_backend_text {
    float single_character_size_in_logical_screen;
    C2D_TextBuf text_buf;
    C2D_Text text;
};


void trans_size(float *w, float *h);
void trans_pos(float *x, float *y);
float offset_3d(enum cpymo_backend_image_draw_type type);

extern float game_width;

const static float text_scale_divisor = 28.0f;

error_t cpymo_backend_text_create(cpymo_backend_text *out, float *out_w, cpymo_parser_stream_span utf8_string, float single_character_size_in_logical_screen)
{
    struct cpymo_backend_text *t = (struct cpymo_backend_text *)malloc(sizeof(struct cpymo_backend_text));
    if(t == NULL) return CPYMO_ERR_OUT_OF_MEM;

    t->text_buf = C2D_TextBufNew(utf8_string.len + 1);
    if(t->text_buf == NULL) {
        free(t);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    t->single_character_size_in_logical_screen = single_character_size_in_logical_screen;

    char *strbuf = alloca(utf8_string.len + 1);
    cpymo_parser_stream_span_copy(strbuf, utf8_string.len + 1, utf8_string);
    if(C2D_TextFontParse(&t->text, font, t->text_buf, strbuf) == NULL) {
        C2D_TextBufDelete(t->text_buf);
        free(t);
        return CPYMO_ERR_OUT_OF_MEM;
    }

    C2D_TextOptimize(&t->text);

    *out = t;

    float w, h;
    C2D_TextGetDimensions(
        &t->text, 
        single_character_size_in_logical_screen / text_scale_divisor,
        single_character_size_in_logical_screen / text_scale_divisor,
        &w, &h);

    *out_w = w / 400.0f * game_width / 2.0f;

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text t)
{
    struct cpymo_backend_text *tt = (struct cpymo_backend_text *)t;
    C2D_TextBufDelete(tt->text_buf);
    free(t);
}

void cpymo_backend_text_draw(cpymo_backend_text t, float x, float y, cpymo_color col, float alpha, enum cpymo_backend_image_draw_type draw_type)
{
    trans_pos(&x, &y);

    u8 a = (u8)(alpha * 255.0f);
    u32 color = C2D_Color32(col.r, col.g, col.b, a);

    struct cpymo_backend_text *tt = (struct cpymo_backend_text *)t;
    float x_scale = tt->single_character_size_in_logical_screen, 
          y_scale = tt->single_character_size_in_logical_screen;

    x_scale /= text_scale_divisor;
    y_scale /= text_scale_divisor;
    
    trans_size(&x_scale, &y_scale);

    float offset_3d_v = offset_3d(draw_type);

    C2D_DrawText(
        &tt->text, 
        C2D_WithColor | C2D_AtBaseline, 
        x + 1.0f + offset_3d_v, y + 1.0f, 0.0f, 
        x_scale, y_scale, 
        C2D_Color32(255 - col.r, 255 - col.g, 255 - col.b, a));  // shadow

    C2D_DrawText(
        &tt->text, 
        C2D_WithColor | C2D_AtBaseline, 
        x + offset_3d_v, y, 0.0f,
        x_scale, y_scale, color);
}

float cpymo_backend_text_width(cpymo_parser_stream_span text, float logic_size)
{
    char *str = alloca(text.len + 1);
    cpymo_parser_stream_span_copy(str, text.len + 1, text);

    C2D_Text t;
    C2D_TextParse(&t, for_measure, str);

    float w, h;
    C2D_TextGetDimensions(
        &t, 
        logic_size / text_scale_divisor,
        logic_size / text_scale_divisor,
        &w, &h);

    C2D_TextBufClear(for_measure);

    return w / 400.0f * game_width / 2.0f;
}