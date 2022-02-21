#ifndef INCLUDE_CPYMO_AUDIO
#define INCLUDE_CPYMO_AUDIO

#include <stddef.h>
#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#include "cpymo_package.h"
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

	AVPacket *packet;
	AVFrame *frame;
	uint8_t *converted_buf;
	size_t converted_buf_size, converted_buf_all_size;

	size_t converted_frame_current_offset;

	AVIOContext *io_context;
	cpymo_package_stream_reader package_reader;

	int stream_id;
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
	c->converted_frame_current_offset = 0;
	c->io_context = NULL;
}

void cpymo_audio_channel_reset(cpymo_audio_channel *);

error_t cpymo_audio_channel_play_file(
	cpymo_audio_channel *channel, 
	const char *filename,
	const cpymo_package_stream_reader *package_reader,
	bool loop);

void cpymo_audio_init(cpymo_audio_system *);
void cpymo_audio_free(cpymo_audio_system *);

void cpymo_audio_copy_mixed_samples(void * dst, size_t len, cpymo_audio_system *s);
bool cpymo_audio_channel_get_samples(
	void **samples, 
	size_t *in_out_len, 
	size_t channelID, 
	cpymo_audio_system *);

static inline float cpymo_audio_get_channel_volume(size_t cid, cpymo_audio_system *s)
{ return s->channels[cid].volume; }

struct cpymo_engine;
bool cpymo_audio_wait_se(struct cpymo_engine *, float);

error_t cpymo_audio_bgm_play(struct cpymo_engine *e, cpymo_parser_stream_span bgmname, bool loop);
void cpymo_audio_bgm_stop(struct cpymo_engine *e);

#endif
