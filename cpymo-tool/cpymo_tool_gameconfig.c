#include "cpymo_tool_prelude.h"
#include "cpymo_tool_gameconfig.h"
#include <stdio.h>
#include <string.h>

static error_t cpymo_tool_str_replace_n(char **out_str, const char *str)
{
    size_t len = 0;
    const char *str_measure = str;
    while (*str_measure) {
        if (*str_measure == '\n') len += 2;
        else len++;
        str_measure++;
    }

    *out_str = (char *)malloc(len + 1);
    if (*out_str == NULL) return CPYMO_ERR_OUT_OF_MEM;

    size_t write_index = 0;
    size_t str_len = strlen(str);
    for (size_t i = 0; i < str_len; ++i) {
        char c = str[i];
        if (c == '\n') {
            (*out_str)[write_index++] = '\\';
            (*out_str)[write_index++] = 'n';
        }
        else {
            (*out_str)[write_index++] = c;
        }
    }

   (*out_str)[write_index] = '\0';

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_gameconfig_write_to_file(
    const char *path,
    const cpymo_gameconfig *gameconfig)
{
    FILE *o = fopen(path, "wb");
    if (o == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

    char *gametitle = NULL;
    error_t err = cpymo_tool_str_replace_n(&gametitle, gameconfig->gametitle);
    if (err != CPYMO_ERR_SUCC) {
        fclose(o);
        return err;
    }

    fprintf(o, "gametitle,%s\n", gametitle);
    free(gametitle);
    fprintf(o, "platform,%s\n", gameconfig->platform);
    fprintf(o, "engineversion,%d.%d\n",
        gameconfig->engineversion.major,
        gameconfig->engineversion.minor);
    fprintf(o, "scripttype,pymo\n");
    fprintf(o, "bgformat,.%s\n", gameconfig->bgformat);
    fprintf(o, "charaformat,.%s\n", gameconfig->charaformat);
    fprintf(o, "charamaskformat,.%s\n", gameconfig->charamaskformat);
    fprintf(o, "bgmformat,.%s\n", gameconfig->bgmformat);
    fprintf(o, "seformat,.%s\n", gameconfig->seformat);
    fprintf(o, "voiceformat,.%s\n", gameconfig->voiceformat);
    fprintf(o, "font,-1\n");
    fprintf(o, "fontsize,%u\n", (unsigned)gameconfig->fontsize);
    fprintf(o, "fontaa,1\n");
    fprintf(o, "hint,%d\n", (int)gameconfig->hint);
    fprintf(o, "prefetching,1\n");
    fprintf(o, "grayselected,%d\n", (int)gameconfig->grayselected);
    fprintf(o, "playvideo,%d\n", (int)gameconfig->playvideo);
    fprintf(o, "textspeed,%d\n", (int)gameconfig->textspeed);
    fprintf(o, "bgmvolume,%d\n", (int)gameconfig->bgmvolume);
    fprintf(o, "vovolume,%d\n", (int)gameconfig->vovolume);
    fprintf(o, "imagesize,%u,%u\n",
        (unsigned)gameconfig->imagesize_w, (unsigned)gameconfig->imagesize_h);
    fprintf(o, "startscript,%s\n", gameconfig->startscript);
    fprintf(o, "nameboxorig,%d,%d\n",
        (int)gameconfig->nameboxorg_x, (int)gameconfig->nameboxorg_y);
    fprintf(o, "cgprefix,%s\n", gameconfig->cgprefix);
    fprintf(o, "textcolor,#%02x%02x%02x\n",
        (int)gameconfig->textcolor.r,
        (int)gameconfig->textcolor.g,
        (int)gameconfig->textcolor.b);
    fprintf(o, "msgtb,%u,%u\n",
        (unsigned)gameconfig->msgtb_t, (unsigned)gameconfig->msgtb_b);
    fprintf(o, "msglr,%u,%u\n",
        (unsigned)gameconfig->msglr_l, (unsigned)gameconfig->msglr_r);
    fprintf(o, "namealign,%s\n",
        gameconfig->namealign == 0 ? "middle" :
            (gameconfig->namealign == 1 ? "left" : "right"));

    fclose(o);
    return CPYMO_ERR_SUCC;
}
