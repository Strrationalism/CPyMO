#include "cpymo_audio.h"
#include <assert.h>
#include <cpymo_backend_audio.h>

void cpymo_audio_channel_reset(cpymo_audio_channel *c)
{
	if (c->swr_context) swr_free(&c->swr_context);
	if (c->codec_context) avcodec_free_context(&c->codec_context);
	if (c->format_context) avformat_close_input(&c->format_context);

	cpymo_audio_channel_init(c);
}

static int cpymo_audio_fmt2ffmpeg(
	int f) {
	switch (f) {
	case cpymo_backend_audio_s16le: return AV_SAMPLE_FMT_S16;
	}

	assert(false);
	return -1;
}

error_t cpymo_audio_channel_play_file(
	cpymo_audio_channel *c, const char * filename, float volume, bool loop)
{
	const cpymo_backend_audio_info *info = 
		cpymo_backend_audio_get_info();
	if (info == NULL) return CPYMO_ERR_SUCC;

	cpymo_audio_channel_reset(c);

	assert(c->enabled == false);

	assert(c->format_context == NULL);
	int result = 
		avformat_open_input(&c->format_context, filename, NULL, NULL);
	if (result != 0) {
		c->format_context = NULL;
		printf("[Error] Can not open %s with error ffmpeg error %s.\n", filename, av_err2str(result));
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	}

	result = avformat_find_stream_info(c->format_context, NULL);
	if (result != 0) {
		cpymo_audio_channel_reset(c);
		printf("[Error] Can not get stream info from %s.\n", filename);
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	int stream_id = av_find_best_stream(
		c->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (result != 0) {
		cpymo_audio_channel_reset(c);
		printf("[Error] Can not find best stream from %s.\n", filename);
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	AVStream *stream = c->format_context->streams[stream_id];
	assert(c->codec_context == NULL);
	const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
	c->codec_context = avcodec_alloc_context3(codec);
	if (c->codec_context == NULL) {
		cpymo_audio_channel_reset(c);
		return CPYMO_ERR_UNKNOWN;
	}

	result = avcodec_open2(c->codec_context, codec, NULL);
	if (result != 0) {
		c->codec_context = NULL;
		cpymo_audio_channel_reset(c);
		return CPYMO_ERR_UNSUPPORTED;
	}

	assert(c->swr_context == NULL);
	c->swr_context = swr_alloc_set_opts(
		NULL,
		av_get_default_channel_layout((int)info->channels),
		cpymo_audio_fmt2ffmpeg(info->format),
		(int)info->freq,
		c->codec_context->channel_layout,
		c->codec_context->sample_fmt,
		c->codec_context->sample_rate,
		0, NULL);
	if (c->swr_context == NULL) {
		cpymo_audio_channel_reset(c);
		return CPYMO_ERR_UNKNOWN;
	}

	c->enabled = true;
	c->volume = volume;
	c->loop = loop;
	return CPYMO_ERR_SUCC;
}

void cpymo_audio_init(cpymo_audio_system *s)
{
	s->enabled = cpymo_backend_audio_get_info() != NULL;

	for (size_t i = 0; i < CPYMO_AUDIO_MAX_CHANNELS; ++i)
		cpymo_audio_channel_init(s->channels + i);
}

void cpymo_audio_free(cpymo_audio_system *s)
{
	for (size_t i = 0; i < CPYMO_AUDIO_MAX_CHANNELS; ++i)
		cpymo_audio_channel_reset(s->channels + i);

	s->enabled = false;
}
