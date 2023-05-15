#include "cpymo_tool_prelude.h"
#include "cpymo_tool_asset_filter.h"


error_t cpymo_tool_asset_filter_init(
    cpymo_tool_asset_filter *filter,
    const char *input_gamedir,
    const char *output_gamedir);

void cpymo_tool_asset_filter_free(
    cpymo_tool_asset_filter *);

static error_t cpymo_tool_asset_filter_run_one_type_asset(
    const struct cpymo_tool_asset_analyzer_string_hashset_item *assets,
    const char *asset_type,
    cpymo_tool_asset_filter_processor filter,
    void *filter_userdata,
    const char *input_gamedir,
    cpymo_package *input_package,
    const char *output_gamedir,
    bool output_to_package);

error_t cpymo_tool_asset_filter_run(
    cpymo_tool_asset_filter *);

error_t cpymo_tool_asset_filter_function_copy(
    cpymo_tool_asset_filter_io *io,
    void *null)
{
    char *data;
    size_t len;
    if (io->input_is_package) {
        len = io->input.package.file_index.file_length;
        data = (char *)malloc(io->input.package.file_index.file_length);

        if (data == NULL) return CPYMO_ERR_OUT_OF_MEM;
        error_t err = cpymo_package_read_file_from_index(
            data, io->input.package.pkg, &io->input.package.file_index);

        if (err != CPYMO_ERR_SUCC) {
            free(data);
            return err;
        }
    }
    else {
        error_t e = cpymo_utils_loadfile(io->input.file.path, &data, &len);
        CPYMO_THROW(e);
    }

    if (io->output_to_package) {
        io->output.package.buf = data;
        io->output.package.len = len;
        io->output.package.mask_buf = io->input_mask_file_buf;
        io->output.package.mask_len = io->input_mask_len;
    }
    else {
        FILE *f = fopen(io->output.file.target_file_path, "wb");
        if (f == NULL) {
            free(data);
            printf(
                "[Error] Can not write file: %s.\n",
                io->output.file.target_file_path);
            return CPYMO_ERR_CAN_NOT_OPEN_FILE;
        }

        size_t len_write = fwrite(data, len, 1, f);
        fclose(f);
        free(data);

        if (len_write != len) {
            printf(
                "[Error] Can not write file: %s.\n",
                io->output.file.target_file_path);
            return CPYMO_ERR_CAN_NOT_OPEN_FILE;
        }

        if (io->input_mask_file_buf) {
            f = fopen(io->output.file.target_mask_path, "wb");
            if (f == NULL) {
                printf(
                    "[Warning] Can not open target mask file: %s\n",
                    io->output.file.target_mask_path);
                return CPYMO_ERR_SUCC;
            }

            len_write = fwrite(io->input_mask_file_buf, io->input_mask_len, 1, f);
            fclose(f);

            if (len_write != io->input_mask_len) {
                printf(
                    "[Warning] Can not write mask file: %s\n",
                    io->output.file.target_mask_path);
                return CPYMO_ERR_SUCC;
            }
        }
    }

    return CPYMO_ERR_SUCC;
}
