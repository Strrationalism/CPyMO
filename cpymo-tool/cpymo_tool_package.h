#ifndef INCLUDE_CPYMO_TOOL_PACKAGE
#define INCLUDE_CPYMO_TOOL_PACKAGE

#include <stdint.h>
#include <stdio.h>
#include "../cpymo/cpymo_str.h"

typedef struct {
    uint32_t max_file_count, current_file_count;
    uint64_t index_section_start_offset, data_section_start_offset;
    FILE *stream;
} cpymo_tool_package_packer;

void cpymo_tool_package_packer_open(
    cpymo_tool_package_packer *packer,
    const char *path,
    size_t max_files_count);

error_t cpymo_tool_package_packer_add_data(
    cpymo_tool_package_packer *packer,
    cpymo_str name,
    void *data,
    size_t len);

error_t cpymo_tool_package_packer_add_file(
    cpymo_tool_package_packer *packer,
    const char *file);

void cpymo_tool_package_packer_close(
    cpymo_tool_package_packer *packer);

#endif
