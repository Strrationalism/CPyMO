#include "cpymo_prelude.h"
#include "cpymo_gameconfig.h"
#include "cpymo_utils.h"
#include "cpymo_parser.h"
#include <memory.h>
#include <stdlib.h>
#include <string.h>

const cpymo_pymo_version cpymo_pymo_version_current = 
	{ 1, 2 };

static void cpymo_dispatch_gameconfig(cpymo_gameconfig *o, cpymo_str key, cpymo_parser *parser) 
{
	size_t magic_key_len;
	#define SETUP(FIELD, cpymo_str) \
		cpymo_str_copy(o->FIELD, sizeof(o->FIELD), cpymo_str)
	#define SETUP_EXT(FIELD, cpymo_str) \
		if(cpymo_str.begin[0] == '.') { cpymo_str.begin++; cpymo_str.len--; } SETUP(FIELD, cpymo_str)
	#define D(KEY) \
		magic_key_len = strlen(KEY); \
		if (magic_key_len == key.len && \
			0 == strncmp(KEY,key.begin , magic_key_len))

	D("gametitle") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP(gametitle, cpymo_str);
		cpymo_utils_replace_str_newline_n(o->gametitle);
		return;
	}

	D("engineversion") {
		cpymo_str major = 
			cpymo_parser_curline_readuntil(parser, '.');

		cpymo_str minor = 
			cpymo_parser_curline_readuntil(parser, '.');

		o->engineversion.major = cpymo_str_atoi(major);
		o->engineversion.minor = cpymo_str_atoi(minor);

		return;
	}

	D("bgformat") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP_EXT(bgformat, cpymo_str);
		return;
	}

	D("charaformat") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP_EXT(charaformat, cpymo_str);
		return;
	}

	D("charamaskformat") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP_EXT(charamaskformat, cpymo_str);
		return;
	}

	D("bgmformat") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP_EXT(bgmformat, cpymo_str);
		return;
	}

	D("seformat") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP_EXT(seformat, cpymo_str);
		return;
	}

	D("voiceformat") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP_EXT(voiceformat, cpymo_str);
		return;
	}

	D("fontsize") {
		o->fontsize = cpymo_utils_clamp(cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)), 4, 1024);
		return;
	}

	D("hint") {
		o->hint = cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)) > 0;
		return;
	}

	D("grayselected") {
		o->hint = cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)) > 0;
		return;
	}

	D("playvideo") {
		o->playvideo = cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)) > 0;
		return;
	}

	D("textspeed") {
		o->textspeed = cpymo_utils_clamp(cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)), 0, 5);
		return;
	}

	D("bgmvolume") {
		o->bgmvolume = cpymo_utils_clamp(cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)), 0, 10);
		return;
	}

	D("vovolume") {
		o->vovolume = cpymo_utils_clamp(cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)), 0, 10);
		return;
	}

	D("startscript") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP(startscript, cpymo_str);
		return;
	}

	D("nameboxorig") {
		o->nameboxorg_x = cpymo_utils_clamp(cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)), -1024, 1024);
		o->nameboxorg_y = cpymo_utils_clamp(cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser)), -1024, 1024);
		return;
	}

	D("cgprefix") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		SETUP(cgprefix, cpymo_str);
		return;
	}

	D("textcolor") {
		o->textcolor = cpymo_str_as_color(
			cpymo_parser_curline_pop_commacell(parser));
		return;
	}

	D("msgtb") {
		o->msgtb_t = cpymo_str_atoi(cpymo_parser_curline_pop_commacell(parser));
		o->msgtb_b = cpymo_str_atoi(cpymo_parser_curline_pop_commacell(parser));
		return;
	}

	D("msglr") {
		o->msglr_l = cpymo_str_atoi(cpymo_parser_curline_pop_commacell(parser));
		o->msglr_r = cpymo_str_atoi(cpymo_parser_curline_pop_commacell(parser));
		return;
	}

	D("namealign") {
		cpymo_str cpymo_str = cpymo_parser_curline_pop_commacell(parser);
		if (cpymo_str.len >= 1) {
			if (cpymo_str.begin[0] == 'l' || cpymo_str.begin[0] == 'L') o->namealign = 1;
			else if (cpymo_str.begin[0] == 'r' || cpymo_str.begin[0] == 'R') o->namealign = 2;
		}
		return;
	}

	D("imagesize") {
		o->imagesize_w = cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser));
		o->imagesize_h = cpymo_str_atoi(
			cpymo_parser_curline_pop_commacell(parser));

		if (o->imagesize_w == 0) o->imagesize_w = 800;
		if (o->imagesize_h == 0) o->imagesize_h = 600;

		return;
	}

	D("platform") {
		cpymo_str plat = cpymo_parser_curline_pop_commacell(parser);
		cpymo_str_trim(&plat);
		SETUP(platform, plat);
		return;
	}

	D("scripttype") {
		cpymo_str s = cpymo_parser_curline_pop_commacell(parser);
		cpymo_str_trim(&s);
		SETUP(scripttype, s);
		return;
	}

	#undef SETUP
	#undef SETUP_EXT
	#undef D
}

error_t cpymo_gameconfig_parse(cpymo_gameconfig *out_config, const char *stream, size_t len)
{
	memset(out_config, 0, sizeof(cpymo_gameconfig));

	/*** Default Config ***/

	out_config->hint = 1;
	out_config->textspeed = 3;
	out_config->textcolor = cpymo_color_white;
	out_config->msgtb_t = 6;
	out_config->msgtb_b = 0;
	out_config->msglr_l = 10;
	out_config->msglr_r = 7;
	strcpy(out_config->platform, "pygame");
	out_config->bgmvolume = 0;
	out_config->vovolume = 0;
	strcpy(out_config->cgprefix, "EV_");
	out_config->grayselected = 1;
	out_config->namealign = 1;
	strcpy(out_config->bgmformat, "mp3");
	strcpy(out_config->voiceformat, "mp3");
	strcpy(out_config->seformat, "mp3");

	/*** Default Config End ***/

	cpymo_parser parser;
	cpymo_parser_init(&parser, stream, len);

	do {
		cpymo_str key = cpymo_parser_curline_pop_commacell(&parser);
		cpymo_dispatch_gameconfig(out_config, key, &parser);

	} while (cpymo_parser_next_line(&parser));

	return CPYMO_ERR_SUCC;
}

error_t cpymo_gameconfig_parse_from_file(
	cpymo_gameconfig *out_config, const char * path)
{
	char *buf = NULL;
	size_t len;
	error_t err = cpymo_utils_loadfile(path, &buf, &len);
	if (err != CPYMO_ERR_SUCC) return err;

	cpymo_utils_replace_cr(buf, len);

	err = cpymo_gameconfig_parse(out_config, buf, len);

	free(buf);

	return err;
}

