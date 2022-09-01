#include <cpymo_prelude.h>
#include "cpymo_backend_software.h"

cpymo_backend_software_context 
    *cpymo_backend_software_cur_context = NULL;

void cpymo_backend_software_set_context(
    cpymo_backend_software_context *context)
{ cpymo_backend_software_cur_context = context; }

