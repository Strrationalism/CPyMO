#include <stdbool.h>
#include <cpymo_error.h>

error_t cpymo_tool_resize_image(
	const char *input_file, const char *output_file, 
	double ratio_w, double ratio_h,
	bool load_mask, bool create_mask,
	const char *out_format);

error_t cpymo_tool_resize_image_package(
	const char *input_file, const char *output_file,
	double ratio_w, double ratio_h,
	bool load_mask, bool create_mask,
	const char *out_format);
