#ifndef INCLUDE_CPYMO_RMENU
#define INCLUDE_CPYMO_RMENU

#include "cpymo_error.h"

#if !defined NON_VISUALLY_IMPAIRED_HELP && defined __ANDROID__
#include <stdint.h>
#include <cpymo_backend_audio.h>
#endif

struct cpymo_engine;
error_t cpymo_rmenu_enter(struct cpymo_engine *);

#endif
