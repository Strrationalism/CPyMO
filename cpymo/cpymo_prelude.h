#ifndef INCLUDE_CPYMO_PRELUDE
#define INCLUDE_CPYMO_PRELUDE

#ifdef LEAKCHECK
#include <stb_leakcheck.h>
#endif

#ifndef CPYMO_FEATURE_LEVEL
#define CPYMO_FEATURE_LEVEL 0
#endif

#if CPYMO_FEATURE_LEVEL >= 1

#if defined DISABLE_AUTOSAVE || defined LOW_FRAME_RATE
#error "Feature Level >= 1 not compatible DISABLE_AUTOSAVE and LOW_FRAMERATE."
#endif

#endif

#endif