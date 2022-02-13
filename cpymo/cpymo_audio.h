#ifndef INCLUDE_CPYMO_AUDIO
#define INCLUDE_CPYMO_AUDIO

#include <stddef.h>
#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#include "cpymo_error.h"

#define CPYMO_AUDIO_MAX_CHANNELS 3
#define CPYMO_AUDIO_CHANNEL_BGM 0
#define CPYMO_AUDIO_CHANNEL_SE 1
#define CPYMO_AUDIO_CHANNEL_VO 2

typedef struct {
	bool enabled, loop;

	float volume;
	
	AVFormatContext *format_context;
	AVCodecContext *codec_context;
	SwrContext *swr_context;
} cpymo_audio_channel;

typedef struct {
	bool enabled;
	cpymo_audio_channel channels[CPYMO_AUDIO_MAX_CHANNELS];
} cpymo_audio_system;

static inline void cpymo_audio_channel_init(cpymo_audio_channel *c)
{
	c->enabled = false;
	c->loop = false;
	c->format_context = NULL;
	c->codec_context = NULL;
	c->swr_context = NULL;
	c->volume = 1.0f;
}

void cpymo_audio_channel_reset(cpymo_audio_channel *);

error_t cpymo_audio_channel_play_file(
	cpymo_audio_channel *channel, 
	const char *filename,
	float volume,
	bool loop);

void cpymo_audio_init(cpymo_audio_system *);
void cpymo_audio_free(cpymo_audio_system *);

#endif