#ifndef INCLUDE_CPYMO_GAMECONFIG
#define INCLUDE_CPYMO_GAMECONFIG

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "cpymo_error.h"
#include "cpymo_color.h"


typedef struct {
	uint16_t major;
	uint16_t minor;
} cpymo_pymo_version;

extern const cpymo_pymo_version cpymo_pymo_version_current;

static inline bool cpymo_pymo_version_compatible(
	cpymo_pymo_version game, cpymo_pymo_version engine) {
	return
		(game.major < engine.major) ||
		(game.major == engine.major && game.minor <= engine.minor);
}

typedef struct {
	char gametitle[256];
	char startscript[64];
	char cgprefix[64];
	char bgformat[4];			// without '.'
	char charaformat[4];		// without '.'
	char charamaskformat[4];	// without '.'
	char bgmformat[4];			// without '.'
	char seformat[4];			// without '.'
	char voiceformat[4];		// without '.'
	char platform[12];
	char scripttype[4];
	uint16_t fontsize;
	int16_t nameboxorg_x, nameboxorg_y;
	uint16_t msgtb_t, msgtb_b;
	uint16_t msglr_l, msglr_r;
	uint16_t imagesize_w, imagesize_h;
	cpymo_color textcolor;

	//unsigned fontaa : 1;
	unsigned hint : 1;
	unsigned grayselected : 1;
	unsigned playvideo : 1;
	unsigned textspeed : 3;
	unsigned bgmvolume : 4;
	unsigned vovolume : 4;
	unsigned namealign : 2;	// 0 - middle, 1 - left, 2 - right

	cpymo_pymo_version engineversion;
} cpymo_gameconfig;
error_t cpymo_gameconfig_parse(cpymo_gameconfig *out_config, const char *stream, size_t len);
error_t cpymo_gameconfig_parse_from_file(cpymo_gameconfig *out_config, const char *path);

static inline bool cpymo_gameconfig_is_symbian(const cpymo_gameconfig *g)
{
	return
		(g->platform[0] == 's' || g->platform[0] == 'S')
		&& g->platform[1] == '6'
		&& g->platform[2] == '0'
		&& (g->platform[3] == 'v' || g->platform[3] == 'V');
}

static inline bool cpymo_gameconfig_scripttype_is_pymo(const cpymo_gameconfig *g)
{
	return
		g->scripttype[0] == 'p'
		&& g->scripttype[1] == 'y'
		&& g->scripttype[2] == 'm'
		&& g->scripttype[3] == 'o';
}

static inline float cpymo_gameconfig_font_size(const cpymo_gameconfig *c)
{ return c->fontsize / 240.0f * c->imagesize_h * 0.8f; }

#endif
