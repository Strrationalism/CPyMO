#include <cpymo_error.h>
#include <stdint.h>

extern error_t cpymo_tool_unpack(const char *pak_path, const char *extension, const char *out_path);
extern error_t cpymo_tool_pack(const char *out_pack_path, const char **files_to_pack, uint32_t file_count);