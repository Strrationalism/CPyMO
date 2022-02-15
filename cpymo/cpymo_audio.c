#include "cpymo_audio.h"
#include <assert.h>
#include <cpymo_backend_audio.h>

static void cpymo_audio_channel_reset_unsafe(cpymo_audio_channel *c)
{
	if (c->swr_context) swr_free(&c->swr_context);
	if (c->codec_context) avcodec_free_context(&c->codec_context);
	if (c->format_context) avformat_close_input(&c->format_context);

	cpymo_audio_channel_init(c);
}

void cpymo_audio_channel_reset(cpymo_audio_channel *c)
{
	cpymo_backend_audio_lock();
	cpymo_audio_channel_reset_unsafe(c);
	cpymo_backend_audio_unlock();
}

static int cpymo_audio_fmt2ffmpeg(
	int f) {
	switch (f) {
	case cpymo_backend_audio_s16le: return AV_SAMPLE_FMT_S16;
	}

	assert(false);
	return -1;
}

static error_t cpymo_audio_channel_grow_convert_buffer(
	cpymo_audio_channel *c, 
	size_t samples,
	const cpymo_backend_audio_info *info)
{
	const size_t size = av_samples_get_buffer_size(
		NULL,
		(int)info->channels,
		(int)samples,
		cpymo_audio_fmt2ffmpeg(info->format),
		1);

	if (size > c->converted_buf_all_size) {
		c->converted_buf = (uint8_t *)realloc(c->converted_buf, size);
		if (c->converted_buf == NULL) {
			cpymo_audio_channel_reset_unsafe(c);
			return CPYMO_ERR_OUT_OF_MEM;
		}
		c->converted_buf_all_size = size;
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_audio_channel_flush_converter(cpymo_audio_channel *c)
{
	const cpymo_backend_audio_info *info = cpymo_backend_audio_get_info();

	const size_t flush_buffer_samples = 512;
	error_t err = cpymo_audio_channel_grow_convert_buffer(c, flush_buffer_samples, info);
	CPYMO_THROW(err);

	int samples = swr_convert(
		c->swr_context,
		&c->converted_buf,
		(int)flush_buffer_samples,
		NULL,
		0);

	if (samples == 0) return CPYMO_ERR_NO_MORE_CONTENT;
	if (samples < 0) {
		printf("[Warning] swr_convert: %s.", av_err2str(samples));
		return CPYMO_ERR_UNKNOWN;
	}

	c->converted_buf_size = av_samples_get_buffer_size(
		NULL,
		(int)info->channels,
		samples,
		cpymo_audio_fmt2ffmpeg(info->format),
		1);

	c->converted_frame_current_offset = 0;
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_audio_channel_convert_current_frame(cpymo_audio_channel *c)
{
	const cpymo_backend_audio_info *info = cpymo_backend_audio_get_info();

	assert(c->frame->nb_samples != 0);
	
	error_t err = cpymo_audio_channel_grow_convert_buffer(
		c, (size_t)c->frame->nb_samples, info);
	CPYMO_THROW(err);

	int samples = swr_convert(
		c->swr_context,
		&c->converted_buf,
		c->frame->nb_samples,
		(const uint8_t **)c->frame->data,
		c->frame->nb_samples);

	assert(samples == c->frame->nb_samples);

	if (samples < 0) {
		const char *err = av_err2str(samples);
		printf("[Warning] swr_convert: %s.", err);
		return CPYMO_ERR_UNKNOWN;
	}

	c->converted_buf_size = av_samples_get_buffer_size(
		NULL,
		(int)info->channels,
		samples,
		cpymo_audio_fmt2ffmpeg(info->format),
		1);

	c->converted_frame_current_offset = 0;
	return CPYMO_ERR_SUCC;
}

static void cpymo_audio_channel_seek_to_head(cpymo_audio_channel *c)
{
	av_seek_frame(c->format_context, -1, 0, AVSEEK_FLAG_FRAME);
}

static error_t cpymo_audio_channel_next_frame(cpymo_audio_channel *c)
{ RETRY: {
	int result = avcodec_receive_frame(c->codec_context, c->frame);

	if (result == 0) {
		// One frame received
		return cpymo_audio_channel_convert_current_frame(c);
	}
	else if (result == AVERROR(EAGAIN)) {
		// No frame received, send more packet to codec.
		result = av_read_frame(c->format_context, c->packet);
		if (result == 0) {
			result = avcodec_send_packet(c->codec_context, c->packet);
			av_packet_unref(c->packet);
			if (result != 0) {
				printf("[Error] avcodec_send_packet: %s.\n", av_err2str(result));
				return CPYMO_ERR_UNKNOWN;
			}
			goto RETRY;
		}
		else if (result == AVERROR_EOF) {
			result = avcodec_send_packet(c->codec_context, NULL);
			if (result != 0) {
				printf("[Error] avcodec_send_packet: %s.\n", av_err2str(result));
				return CPYMO_ERR_UNKNOWN;
			}
			goto RETRY;
		}
		else {
			printf("[Error] av_read_frame: %s.\n", av_err2str(result));
			return CPYMO_ERR_UNKNOWN;
		}
	}
	else if (result == AVERROR_EOF) {
		// No frame received, and no more packet send to codec.
		// Flush Swr buffer.
		return cpymo_audio_channel_flush_converter(c);
	}
	else {
		printf("[Error] av_receive_frame: %s.\n", av_err2str(result));
		return CPYMO_ERR_UNKNOWN;
	}
}}

static void cpymo_audio_channel_write_samples(uint8_t *dst, size_t len, cpymo_audio_channel *c)
{
	while (len > 0) {
		uint8_t *src = c->converted_buf + c->converted_frame_current_offset;
		size_t src_size = c->converted_buf_size - c->converted_frame_current_offset;

		if (src_size == 0) {
			error_t err = cpymo_audio_channel_next_frame(c);
			if (err == CPYMO_ERR_SUCC) 
				continue;
			else if (err == CPYMO_ERR_NO_MORE_CONTENT) {
				if (c->loop) {
					cpymo_audio_channel_seek_to_head(c);
					continue;
				}
				else {
					goto FILL_BLANK_AND_RESET;
				}
			}
			else {
				goto FILL_BLANK_AND_RESET;
			}
		}
		else {
			size_t write_size = src_size;
			if (write_size > len) write_size = len;

			memcpy(dst, src, write_size);
			c->converted_frame_current_offset += write_size;
			dst += write_size;
			len -= write_size;
		}
	}

	return;

FILL_BLANK_AND_RESET:
	memset(dst, 0, len);
	cpymo_audio_channel_reset_unsafe(c);
	return;
}

error_t cpymo_audio_channel_play_file(
	cpymo_audio_channel *c, const char * filename, float volume, bool loop)
{
	const cpymo_backend_audio_info *info = 
		cpymo_backend_audio_get_info();
	if (info == NULL) return CPYMO_ERR_SUCC;

	cpymo_backend_audio_lock();

	cpymo_audio_channel_reset_unsafe(c);

	assert(c->enabled == false);

	assert(c->format_context == NULL);
	int result = 
		avformat_open_input(&c->format_context, filename, NULL, NULL);
	if (result != 0) {
		c->format_context = NULL;
		printf("[Error] Can not open %s with error ffmpeg error %s.\n", filename, av_err2str(result));
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	}

	result = avformat_find_stream_info(c->format_context, NULL);
	if (result != 0) {
		cpymo_audio_channel_reset_unsafe(c);
		printf("[Error] Can not get stream info from %s.\n", filename);
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	int stream_id = av_find_best_stream(
		c->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (result != 0) {
		cpymo_audio_channel_reset_unsafe(c);
		printf("[Error] Can not find best stream from %s.\n", filename);
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	AVStream *stream = c->format_context->streams[stream_id];
	assert(c->codec_context == NULL);
	const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
	c->codec_context = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(c->codec_context, stream->codecpar);
	c->codec_context->pkt_timebase = stream->time_base;
	if (c->codec_context == NULL) {
		cpymo_audio_channel_reset_unsafe(c);
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_UNKNOWN;
	}

	result = avcodec_open2(c->codec_context, codec, NULL);
	if (result != 0) {
		c->codec_context = NULL;
		cpymo_audio_channel_reset_unsafe(c);
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_UNSUPPORTED;
	}

	assert(c->swr_context == NULL);
	c->swr_context = swr_alloc_set_opts(
		NULL,
		av_get_default_channel_layout((int)info->channels),
		cpymo_audio_fmt2ffmpeg(info->format),
		(int)info->freq,
		stream->codecpar->channel_layout,
		stream->codecpar->format,
		stream->codecpar->sample_rate,
		0, NULL);
	if (c->swr_context == NULL) {
		cpymo_audio_channel_reset_unsafe(c);
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_UNKNOWN;
	}

	result = swr_init(c->swr_context);
	if (result < 0) {
		cpymo_audio_channel_reset_unsafe(c);
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_UNKNOWN;
	}

	if (c->packet == NULL) {
		c->packet = av_packet_alloc();
		if (c->packet == NULL) {
			cpymo_audio_channel_reset_unsafe(c);
			cpymo_backend_audio_unlock();
			return CPYMO_ERR_OUT_OF_MEM;
		}
	}

	if (c->frame == NULL) {
		c->frame = av_frame_alloc();
		if (c->frame == NULL) {
			cpymo_audio_channel_reset_unsafe(c);
			cpymo_backend_audio_unlock();
			return CPYMO_ERR_OUT_OF_MEM;
		}
	}

	c->enabled = true;
	c->volume = volume;
	c->loop = loop;
	c->converted_frame_current_offset = 0;

	// read first frame
	if (cpymo_audio_channel_next_frame(c) != CPYMO_ERR_SUCC) {
		cpymo_audio_channel_reset_unsafe(c);
		cpymo_backend_audio_unlock();
		return CPYMO_ERR_SUCC;
	}

	cpymo_backend_audio_unlock();
	return CPYMO_ERR_SUCC;
}

void cpymo_audio_init(cpymo_audio_system *s)
{
	s->enabled = cpymo_backend_audio_get_info() != NULL;

	for (size_t i = 0; i < CPYMO_AUDIO_MAX_CHANNELS; ++i) {
		cpymo_audio_channel_init(s->channels + i);
		s->channels[i].packet = NULL;
		s->channels[i].frame = NULL;
		s->channels[i].converted_buf = NULL;
		s->channels[i].converted_buf_all_size = 0;
	}
}

void cpymo_audio_free(cpymo_audio_system *s)
{
	if (s->enabled == false) return;

	for (size_t i = 0; i < CPYMO_AUDIO_MAX_CHANNELS; ++i) {
		cpymo_audio_channel_reset(s->channels + i);

		if (s->channels[i].packet) av_packet_free(&s->channels[i].packet);
		if (s->channels[i].frame) av_frame_free(&s->channels[i].frame);
		if (s->channels[i].converted_buf) free(s->channels[i].converted_buf);
	}

	s->enabled = false;
}

void cpymo_audio_copy_samples(void * dst, size_t len, cpymo_audio_system *s)
{
	if (s->enabled == false) return;

	for (size_t i = 0; i < CPYMO_AUDIO_MAX_CHANNELS; ++i) {
		if (s->channels[i].enabled) {
			cpymo_audio_channel_write_samples(dst, len, s->channels + i);
		}
	}
}
