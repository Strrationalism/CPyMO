#include "cpymo_backend_software.h"

cpymo_backend_image_software_context 
    *cpymo_backend_image_software_cur_context = NULL;

void cpymo_backend_image_software_set_context(
    cpymo_backend_image_software_context *context)
{ cpymo_backend_image_software_cur_context = context; }

