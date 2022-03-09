#include "cpymo_movie.h"
#include "cpymo_engine.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct {
	AVFormatContext *format_context;
	int video_stream_index, audio_stream_index;

	AVCodecContext *video_codec_context, *audio_codec_context;

	AVPacket *packet;
	AVFrame *video_frame, *audio_frame;

	bool no_more_content;
} cpymo_movie;

static error_t cpymo_movie_send_packets(cpymo_movie *m)
{
	RETRY:
	if (m->no_more_content) return CPYMO_ERR_NO_MORE_CONTENT;

	int err = av_read_frame(m->format_context, m->packet);
	if (err == AVERROR_EOF) {
		m->no_more_content = true;

		err = avcodec_send_packet(m->audio_codec_context, NULL);
		if (err < 0) {
			printf("[Error] Could not flush video codec: %s.\n", av_err2str(err));
		}

		int err2 = avcodec_send_packet(m->video_codec_context, NULL);
		if (err2 < 0) {
			printf("[Error] Could not flush audio codec: %s.\n", av_err2str(err));
		}

		if (err < 0 || err2 < 0) {
			return CPYMO_ERR_UNKNOWN;
		}
	}
	else if (err == 0) {
		if (m->packet->stream_index == m->audio_stream_index && m->audio_codec_context) {
			err = avcodec_send_packet(m->audio_codec_context, m->packet);
			av_packet_unref(m->packet);
		}
		else if (m->packet->stream_index == m->video_stream_index) {
			err = avcodec_send_packet(m->video_codec_context, m->packet);
			av_packet_unref(m->packet);
		}
		else {
			av_packet_unref(m->packet);
			goto RETRY;
		}

		if (err < 0) {
			printf("[Error] Can not send packet to codec: %s.\n", av_err2str(err));
			return CPYMO_ERR_UNKNOWN;
		}
	}
	else {
		printf("[Error] Could not read packet: %s.\n", av_err2str(err));
		return CPYMO_ERR_UNKNOWN;
	}
	
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_movie_update(cpymo_engine *e, void *ui_data, float dt)
{
	cpymo_ui_exit(e);
	return CPYMO_ERR_SUCC;
}

static void cpymo_movie_draw(const cpymo_engine *e, const void *ui_data)
{

}

static void cpymo_movie_delete(cpymo_engine *e, void *ui_data)
{
	cpymo_movie *m = (cpymo_movie *)ui_data;

	if (m->audio_frame) av_frame_free(&m->audio_frame);
	if (m->video_frame) av_frame_free(&m->video_frame);
	if (m->packet) av_packet_free(&m->packet);
	if (m->audio_codec_context) avcodec_free_context(&m->audio_codec_context);
	if (m->video_codec_context) avcodec_free_context(&m->video_codec_context);
	if (m->format_context) avformat_close_input(&m->format_context);
}

error_t cpymo_movie_play(cpymo_engine * e, const char *path)
{
	cpymo_movie *m = NULL;
	error_t err = 
		cpymo_ui_enter(
			(void **)&m, 
			e, 
			sizeof(*m), 
			&cpymo_movie_update, 
			&cpymo_movie_draw, 
			&cpymo_movie_delete);
	CPYMO_THROW(err);

	m->format_context = NULL;
	m->video_codec_context = NULL;
	m->audio_codec_context = NULL;
	m->no_more_content = false;
	m->audio_frame = NULL;
	m->video_frame = NULL;

	#define THROW(ERR_COND, ERRCODE, MESSAGE) \
		if (ERR_COND) { \
			printf("[Error] %s: %s.\n", MESSAGE, path); \
			cpymo_ui_exit(e); \
			return ERRCODE; \
		}

	#define THROW_AVERR(ERR_COND, ERRCODE) \
		THROW(ERR_COND, ERRCODE, av_err2str(averr));

	int averr = avformat_open_input(&m->format_context, path, NULL, NULL);
	THROW_AVERR(averr, CPYMO_ERR_CAN_NOT_OPEN_FILE);

	averr = avformat_find_stream_info(m->format_context, NULL);
	THROW_AVERR(averr < 0, CPYMO_ERR_NOT_FOUND);

	m->video_stream_index = av_find_best_stream(m->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	THROW(m->video_stream_index < 0, CPYMO_ERR_NOT_FOUND, "Could not find video stream");

	const AVCodec *video_codec = 
		avcodec_find_decoder(m->format_context->streams[m->video_stream_index]->codecpar->codec_id);
	THROW(video_codec == NULL, CPYMO_ERR_NOT_FOUND, "Could not find video codec");

	m->video_codec_context = avcodec_alloc_context3(video_codec);
	THROW(m->video_codec_context == NULL, CPYMO_ERR_UNKNOWN, "Could not open video codec context");

	averr = avcodec_open2(m->video_codec_context, video_codec, NULL);
	THROW_AVERR(averr, CPYMO_ERR_UNKNOWN);

	averr = avcodec_parameters_to_context(
		m->video_codec_context, 
		m->format_context->streams[m->video_stream_index]->codecpar);
	THROW_AVERR(averr < 0, CPYMO_ERR_UNKNOWN);

	m->audio_stream_index = av_find_best_stream(m->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (m->audio_stream_index >= 0 && e->audio.enabled) {
		const AVCodec *audio_codec = 
			avcodec_find_decoder(
				m->format_context->streams[m->audio_stream_index]->codecpar->codec_id);
		if (audio_codec == NULL) goto AUDIO_FAILED;

		m->audio_codec_context = avcodec_alloc_context3(audio_codec);
		if (m->audio_codec_context == NULL) goto AUDIO_FAILED;

		if (avcodec_open2(m->audio_codec_context, audio_codec, NULL)) {
			avcodec_free_context(&m->audio_codec_context);
			goto AUDIO_FAILED;
		}

		if (avcodec_parameters_to_context(
			m->audio_codec_context,
			m->format_context->streams[m->audio_stream_index]->codecpar)) {
			avcodec_free_context(&m->audio_codec_context);
			goto AUDIO_FAILED;
		}

		m->audio_frame = av_frame_alloc();
		if (m->audio_frame == NULL) {
			avcodec_free_context(&m->audio_codec_context);
			goto AUDIO_FAILED;
		}
	}
	else {
		AUDIO_FAILED:
		m->audio_codec_context = NULL;
		m->audio_frame = NULL;
	}

	m->packet = av_packet_alloc();
	THROW(m->packet == NULL, CPYMO_ERR_OUT_OF_MEM, "Could not alloc AVPacket");

	m->video_frame = av_frame_alloc();
	THROW(m->video_frame == NULL, CPYMO_ERR_OUT_OF_MEM, "Could not alloc AVFrame");

	do {
		err = cpymo_movie_send_packets(m);
	} while (err != CPYMO_ERR_SUCC && err != CPYMO_ERR_NO_MORE_CONTENT);

	return CPYMO_ERR_SUCC;
}
