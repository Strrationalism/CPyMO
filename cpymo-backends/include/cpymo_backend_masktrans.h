#ifndef INCLUDE_CPYMO_BACKEND_MASKTRANS
#define INCLUDE_CPYMO_BACKEND_MASKTRANS

#include "../../cpymo/cpymo_error.h"
#include <stdbool.h>

typedef void *cpymo_backend_masktrans;

// if backend doesn't support masktrans, returns CPYMO_ERR_UNSUPPORTED.
error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h);

void cpymo_backend_masktrans_free(cpymo_backend_masktrans);

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans, float t, bool is_fade_in);


#endif
