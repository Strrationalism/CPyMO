#include <cpymo_backend_text.h>
#include <3ds.h>
#include <citro2d.h>

C2D_Font font;

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

    printf("[Info] Font Loaded!!\n");

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_sys_free()
{
    C2D_FontFree(font);
    cfguExit();
    romfsExit();
}

//error_t cpymo_backend_text_create(cpymo_backend_text *out, const char *utf8_string, float single_character_size_in_logical_screen);
//void cpymo_backend_text_free(cpymo_backend_text);

//void cpymo_backend_text_draw(cpymo_backend_text, float x, float y, cpymo_color col, float alpha);