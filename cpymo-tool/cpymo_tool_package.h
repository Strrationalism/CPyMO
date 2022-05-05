#include <cpymo_error.h>
#include <stdint.h>

error_t cpymo_tool_unpack(const char *pak_path, const char *extension, const char *out_path);
error_t cpymo_tool_pack(const char *out_pack_path, const char **files_to_pack, uint32_t file_count);
error_t cpymo_tool_get_file_list(char ***files, size_t *count, const char *list_file);

int cpymo_tool_invoke_pack(int argc, const char **argv);
int cpymo_tool_invoke_unpack(int argc, const char **argv);