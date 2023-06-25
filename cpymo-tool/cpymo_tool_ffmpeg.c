#include "cpymo_tool_prelude.h"
#include "cpymo_tool_ffmpeg.h"
#include <stdio.h>
#include <string.h>

error_t cpymo_tool_ffmpeg_search(const char **out_ffmpeg_command)
{
    #define TRY(CMD) \
        if (system(CMD " -version") == 0) { \
            *out_ffmpeg_command = CMD; \
            return CPYMO_ERR_SUCC; \
        } \

    #ifdef _WIN32
    TRY("ffmpeg");
    TRY("cmd /c ffmpeg");
    TRY("powershell ./ffmpeg");
    #else
    TRY("./ffmpeg");
    TRY("ffmpeg");
    #endif

    #undef TRY

    return CPYMO_ERR_NOT_FOUND;
}

error_t cpymo_tool_ffmpeg_call(
    const char *ffmpeg_command,
    const char *src,
    const char *dst,
    const char *fmt,
    const char *flags)
{
    size_t flags_len = 0;
    if (flags) flags_len = 1 + strlen(flags);

    char *command = (char *)malloc(
        strlen(ffmpeg_command)
        + strlen(src)
        + strlen(dst)
        + strlen(fmt)
        + flags_len
        + 32);
    if (command == NULL) return CPYMO_ERR_OUT_OF_MEM;

    sprintf(command, "%s -i \"%s\" -y -v quiet -f %s %s%s\"%s\"",
        ffmpeg_command,
        src,
        fmt,
        flags == NULL ? "" : flags,
        flags == NULL ? "" : " ",
        dst);
    system(command);
    free(command);

    return CPYMO_ERR_SUCC;
}
