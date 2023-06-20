#ifndef INCLUDE_CPYMO_TOOL_FFMPEG

#include "../cpymo/cpymo_error.h"

error_t cpymo_tool_ffmpeg_search(const char **out_ffmpeg_command);

error_t cpymo_tool_ffmpeg_call(
    const char *ffmpeg_command,
    const char *src,
    const char *dst,
    const char *fmt);

#endif
