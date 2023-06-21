#include "cpymo_tool_prelude.h"
#include "cpymo_tool_ffmpeg.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define NUL_DEVICE "nul"
#else
#define NUL_DEVICE "/dev/null"
#endif

error_t cpymo_tool_ffmpeg_search(const char **out_ffmpeg_command)
{
    #define TRY(CMD) \
        if (system(CMD " -version > " NUL_DEVICE) == 0) { \
            *out_ffmpeg_command = CMD; \
            return CPYMO_ERR_SUCC; \
        } \

    #ifdef _WIN32
    TRY("powershell ./ffmpeg");
    #else
    TRY("./ffmpeg");
    #endif

    TRY("ffmpeg");

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
        + flags_len
        + 16 + strlen(NUL_DEVICE));
    if (command == NULL) return CPYMO_ERR_OUT_OF_MEM;

    sprintf(command, "%s -i \"%s\" %s%s\"%s\" > " NUL_DEVICE,
        ffmpeg_command,
        src,
        flags == NULL ? "" : flags,
        flags == NULL ? "" : " ",
        dst);
    system(command);
    free(command);

    return CPYMO_ERR_SUCC;
}
