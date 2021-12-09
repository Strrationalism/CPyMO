#ifndef INCLUDE_CPYMO_GAMECONFIG
#define INCLUDE_CPYMO_GAMECONFIG

#include <stdbool.h>
#include <stdint.h>
#include "cpymo_error.h"
#include "cpymo_color.h"

typedef struct {
	char gametitle[64];
	char startscript[64];
	char cgprefix[64];
	char bgformat[4];			// without '.'
	char charaformat[4];		// without '.'
	char charamaskformat[4];	// without '.'
	char bgmformat[4];			// without '.'
	char seformat[4];			// without '.'
	char voiceformat[4];		// without '.'
	uint16_t fontsize;
	uint16_t nameboxorg_x, nameboxorg_y;
	uint16_t msgtb_t, msgtb_b;
	uint16_t msglr_l, msglr_r;
	cpymo_color textcolor;

	unsigned fontaa : 1;
	unsigned hint : 1;
	unsigned grayselected : 1;
	unsigned playvideo : 1;
	unsigned textspeed : 3;
	unsigned bgmvolume : 4;
	unsigned vovolume : 4;
	unsigned namealign : 2;
} cpymo_gameconfig;

#define CPYMO_GAMECONFIG_NAMEALIGN_MIDDLE 0
#define CPYMO_GAMECONFIG_NAMEALIGN_LEFT 1
#define CPYMO_GAMECONFIG_NAMEALIGN_RIGHT 2

error_t cpymo_gameconfig_parse(cpymo_gameconfig *out_config, const char *stream, size_t len);
error_t cpymo_gameconfig_parse_from_file(cpymo_gameconfig *out_config, const char *path);

#endif
