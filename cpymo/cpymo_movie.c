#include "cpymo_movie.h"
#include "cpymo_engine.h"
#include <assert.h>
#include <cpymo_backend_movie.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct {
	AVFormatContext *format_context;
	int video_stream_index;

	AVCodecContext *video_codec_context;

	AVPacket *packet;
	AVFrame *video_frame;

	bool no_more_content;
	bool backend_inited;
	bool skip_pressed;

	float bgm_volume;

	float current_time;
} cpymo_movie;

static error_t cpymo_movie_send_packets(cpymo_movie *m)
{
	RETRY:
	if (m->no_more_content) return CPYMO_ERR_NO_MORE_CONTENT;

	int err = av_read_frame(m->format_context, m->packet);
	if (err == AVERROR_EOF) {
		m->no_more_content = true;

		err = avcodec_send_packet(m->video_codec_context, NULL);
		if (err < 0) {
			printf("[Error] Could not flush audio codec: %s.\n", av_err2str(err));
		}

		if (err < 0) {
			return CPYMO_ERR_UNKNOWN;
		}
	}
	else if (err == 0) {
		if (m->packet->stream_index == m->video_stream_index) {
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

static error_t cpymo_movie_send_video_frame_to_backend(cpymo_movie *m)
{
RETRY: {
	int err = avcodec_receive_frame(m->video_codec_context, m->video_frame);
	if (err == 0) {
		switch (m->video_frame->format) {
		case AV_PIX_FMT_YUV420P:
		case AV_PIX_FMT_YUV422P:
		case AV_PIX_FMT_YUV420P16:
		case AV_PIX_FMT_YUV422P16:
			cpymo_backend_movie_update_yuv_surface(
				m->video_frame->data[0],
				(size_t)m->video_frame->linesize[0],
				m->video_frame->data[1],
				(size_t)m->video_frame->linesize[1],
				m->video_frame->data[2],
				(size_t)m->video_frame->linesize[2]
			);
			break;
		case AV_PIX_FMT_YUYV422:
			cpymo_backend_movie_update_yuyv_surface(
				m->video_frame->data[0],
				(size_t)m->video_frame->linesize[0]
			);
		default: assert(false);
		};

		const float video_current_frame_time =
			(float)
			(m->video_frame->best_effort_timestamp
				* av_q2d(m->format_context->streams[m->video_stream_index]->time_base));

		if (video_current_frame_time < m->current_time)
			goto RETRY;

		return CPYMO_ERR_SUCC;
	}
	else if (err == AVERROR(EAGAIN)) {
		error_t err = cpymo_movie_send_packets(m);
		if (err == CPYMO_ERR_NO_MORE_CONTENT) return CPYMO_ERR_NO_MORE_CONTENT;
		else if (err == CPYMO_ERR_SUCC) { goto RETRY; }
		else {
			printf("[Error] Failed to request more frames.\n");
			goto RETRY;
		}
	}
	else if (err == AVERROR_EOF) { return CPYMO_ERR_NO_MORE_CONTENT; }
	else {
		printf("[Error] Failed to receive frame.\n");
		return CPYMO_ERR_UNKNOWN;
	}
} }

static error_t cpymo_movie_update(cpymo_engine *e, void *ui_data, float dt)
{
	cpymo_movie *m = (cpymo_movie *)ui_data;
	m->current_time += dt;

	const float video_current_frame_time = 
		(float)
		(m->video_frame->best_effort_timestamp 
			* av_q2d(m->format_context->streams[m->video_stream_index]->time_base));

	if (video_current_frame_time < m->current_time) {
		error_t err = cpymo_movie_send_video_frame_to_backend(m);
		if (err == CPYMO_ERR_NO_MORE_CONTENT) {
			cpymo_ui_exit(e);
			return CPYMO_ERR_SUCC;
		}
		else if (err == CPYMO_ERR_SUCC)
			cpymo_engine_request_redraw(e);
	}

	if (CPYMO_INPUT_JUST_RELEASED(e, skip)) {
		if (m->skip_pressed) m->skip_pressed = false;
		else {
			cpymo_ui_exit(e);
			return CPYMO_ERR_SUCC;
		}
	}

	return CPYMO_ERR_SUCC;
}

static void cpymo_movie_draw(const cpymo_engine *e, const void *ui_data)
{
	cpymo_backend_movie_draw_surface();
}

static void cpymo_movie_delete(cpymo_engine *e, void *ui_data)
{
	cpymo_movie *m = (cpymo_movie *)ui_data;

	cpymo_audio_bgm_stop(e);

	e->audio.channels[CPYMO_AUDIO_CHANNEL_BGM].volume = m->bgm_volume;

	if (m->video_frame) av_frame_free(&m->video_frame);
	if (m->packet) av_packet_free(&m->packet);
	if (m->video_codec_context) avcodec_free_context(&m->video_codec_context);
	if (m->format_context) avformat_close_input(&m->format_context);
	if (m->backend_inited) cpymo_backend_movie_free_surface();
}

error_t cpymo_movie_play(cpymo_engine * e, cpymo_parser_stream_span videoname)
{
	cpymo_audio_bgm_stop(e);
	cpymo_audio_se_stop(e);
	cpymo_audio_channel_reset(&e->audio.channels[CPYMO_AUDIO_CHANNEL_VO]);

	switch (cpymo_backend_movie_how_to_play()) {
	case cpymo_backend_movie_how_to_play_unsupported:
		printf("[Warning] This platform does not support video playing.\n");
		return CPYMO_ERR_SUCC;
	case cpymo_backend_movie_how_to_play_send_surface:
		break;
	};
	
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
	m->no_more_content = false;
	m->packet = NULL;
	m->video_frame = NULL;
	m->current_time = 0;
	m->backend_inited = false;
	m->skip_pressed = e->input.skip;
	m->bgm_volume = e->audio.channels[CPYMO_AUDIO_CHANNEL_BGM].volume;
	e->audio.channels[CPYMO_AUDIO_CHANNEL_BGM].volume = 1.0f;

	#define THROW(ERR_COND, ERRCODE, MESSAGE) \
		if (ERR_COND) { \
			if (path) free(path); \
			printf("[Error] %s: %s.\n", MESSAGE, path); \
			cpymo_ui_exit(e); \
			return ERRCODE; \
		}

	#define THROW_AVERR(ERR_COND, ERRCODE) \
		THROW(ERR_COND, ERRCODE, av_err2str(averr));

	char *path = NULL;
	err = cpymo_assetloader_get_video_path(&path, videoname, &e->assetloader);
	THROW(err != CPYMO_ERR_SUCC, err, "Faild to get video path");

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

	averr = avcodec_parameters_to_context(
		m->video_codec_context,
		m->format_context->streams[m->video_stream_index]->codecpar);
	THROW_AVERR(averr < 0, CPYMO_ERR_UNKNOWN);

	averr = avcodec_open2(m->video_codec_context, video_codec, NULL);
	THROW_AVERR(averr, CPYMO_ERR_UNKNOWN);

	m->packet = av_packet_alloc();
	THROW(m->packet == NULL, CPYMO_ERR_OUT_OF_MEM, "Could not alloc AVPacket");

	m->video_frame = av_frame_alloc();
	THROW(m->video_frame == NULL, CPYMO_ERR_OUT_OF_MEM, "Could not alloc AVFrame");

	m->video_frame->best_effort_timestamp = 0;

	int width = m->format_context->streams[m->video_stream_index]->codecpar->width;
	int height = m->format_context->streams[m->video_stream_index]->codecpar->height;

	enum AVPixelFormat format = 
		(enum AVPixelFormat)m->format_context->streams[m->video_stream_index]->codecpar->format;

	enum cpymo_backend_movie_format backend_format;
	switch (format) {
	case AV_PIX_FMT_YUV420P: backend_format = cpymo_backend_movie_format_yuv420p; break;
	case AV_PIX_FMT_YUV422P: backend_format = cpymo_backend_movie_format_yuv422p; break;
	case AV_PIX_FMT_YUV420P16: backend_format = cpymo_backend_movie_format_yuv420p16; break;
	case AV_PIX_FMT_YUV422P16: backend_format = cpymo_backend_movie_format_yuv422p16; break;
	case AV_PIX_FMT_YUYV422: backend_format = cpymo_backend_movie_format_yuyv422; break;
	default: THROW(true, CPYMO_ERR_UNSUPPORTED, "[Error] Movie pixel format unsupported");
	};

	err = cpymo_backend_movie_init_surface((size_t)width, (size_t)height, backend_format);
	THROW(err == CPYMO_ERR_UNSUPPORTED, err, "[Error] Movie backend could not support this video");
	THROW(err != CPYMO_ERR_SUCC, err, "[Error] Movie backend init failed");

	m->backend_inited = true;

	cpymo_audio_channel_play_file(
		&e->audio.channels[CPYMO_AUDIO_CHANNEL_BGM],
		path,
		NULL,
		false);
	
	free(path);

	return CPYMO_ERR_SUCC;
}
