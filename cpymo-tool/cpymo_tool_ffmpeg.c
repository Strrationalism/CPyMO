#include "cpymo_tool_prelude.h"
#include "cpymo_tool_ffmpeg.h"
#include <stdio.h>

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


