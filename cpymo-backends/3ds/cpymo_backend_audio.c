#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cpymo_backend_audio.h>
#include <cpymo_engine.h>

static bool audio_enabled = false;

#define SAMPLERATE 32000
#define SAMPLESPERBUF (SAMPLERATE / 60)
#define BYTEPERSAMPLE 4
#define DSP_FIRM "sdmc:/3ds/dspfirm.cdc"
#define BUFFERS 3

static s16 *audio_buf;

static ndspWaveBuf waveBuf[BUFFERS * CPYMO_AUDIO_MAX_CHANNELS];

extern cpymo_engine engine;

const static cpymo_backend_audio_info audio_info = {
    SAMPLERATE,
    cpymo_backend_audio_s16,
    2
};

const cpymo_backend_audio_info *cpymo_backend_audio_get_info(void)
{
    return audio_enabled ? &audio_info : NULL;
}

bool cpymo_backend_audio_enabled() 
{
    return audio_enabled;
}

static bool cpymo_backend_audio_need_dump_dsp() 
{
    FILE *f = fopen(DSP_FIRM, "rb");
    if(f != NULL) {
        fclose(f);
        return false;
    }
    return true;
}

static inline void cpymo_backend_update_volume(size_t cid) 
{
    static float volume[CPYMO_AUDIO_MAX_CHANNELS] = {0.0f};

    float new_volume = cpymo_audio_get_channel_volume(cid, &engine.audio);
    if(volume[cid] != new_volume) {
        volume[cid] = new_volume;
        
        float mix[12];
        memset(mix + 2, 0, sizeof(mix) - 2 * sizeof(float));
        mix[0] = new_volume;
        mix[1] = new_volume;

        ndspChnSetMix(cid, mix);
    }
}

static inline void cpymo_backend_audio_prepare_buffer(ndspWaveBuf *buf, size_t cid) 
{
    u8 *dst = (u8 *)buf->data_pcm16;
    const size_t buf_size = buf->nsamples * BYTEPERSAMPLE;

    void *samples;
    size_t io_len = buf_size;
    size_t written = 0;

    while(cpymo_audio_channel_get_samples(&samples, &io_len, cid, &engine.audio) && io_len) {
        memcpy(dst + written, samples, io_len);
        written += io_len;
        io_len = buf_size - written;
    }

    if(written != buf_size) {
        memset(dst + written, 0, buf_size - written);
    }
}


static void cpymo_backend_audio_callback(void *_) 
{
    static unsigned double_buffering[CPYMO_AUDIO_MAX_CHANNELS] = {0};
    for(size_t cid = 0; cid < CPYMO_AUDIO_MAX_CHANNELS; ++cid) {
        cpymo_backend_update_volume(cid);

        ndspWaveBuf *wbuf = &waveBuf[BUFFERS * cid + double_buffering[cid]];
        
        while(wbuf->status == NDSP_WBUF_DONE) {
            cpymo_backend_audio_prepare_buffer(wbuf, cid);
            DSP_FlushDataCache(wbuf->data_pcm8, wbuf->nsamples * BYTEPERSAMPLE);
            ndspChnWaveBufAdd(cid, wbuf);
            double_buffering[cid] = (double_buffering[cid] + 1) % BUFFERS;
            wbuf = &waveBuf[BUFFERS * cid + double_buffering[cid]];
        }
    }
}

static void cpymo_backend_audio_callback_donothing(void *_) {}

void cpymo_backend_audio_init()
{
    if(cpymo_backend_audio_need_dump_dsp())
        printf("[Error] DSP Firmware not found, you can dump it using DSP1 (https://github.com/zoogie/DSP1).\n");

    if(R_FAILED(ndspInit())) {
        printf("[Error] Failed to initialize audio.\n");
        return;
    }

    size_t buf_size = SAMPLESPERBUF * BYTEPERSAMPLE;

    audio_buf = (s16 *)linearAlloc(buf_size * BUFFERS * CPYMO_AUDIO_MAX_CHANNELS);
    if(audio_buf == NULL) {
        ndspExit();
        printf("[Error] Failed to alloc audio buf.\n");
        return;
    }

    memset(audio_buf, 0, buf_size * BUFFERS * CPYMO_AUDIO_MAX_CHANNELS);
    memset(waveBuf, 0, sizeof(waveBuf));

    for(int cid = 0; cid < CPYMO_AUDIO_MAX_CHANNELS; ++cid) {
        ndspChnReset(cid);
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspChnSetInterp(cid, NDSP_INTERP_LINEAR);
        ndspChnSetRate(cid, SAMPLERATE);
        ndspChnSetFormat(cid, NDSP_FORMAT_STEREO_PCM16);
        cpymo_backend_update_volume(cid);
        
        for(size_t bufid = 0; bufid < BUFFERS; ++bufid) {
            ndspWaveBuf *buf = &waveBuf[BUFFERS * cid + bufid];

            buf->data_vaddr = ((u8 *)audio_buf) + (cid * BUFFERS + bufid) * buf_size;
            buf->nsamples = SAMPLESPERBUF;            

            ndspChnWaveBufAdd(cid, buf);
        }
    }

    audio_enabled = true;
}

void cpymo_backend_audio_free() 
{
    if(audio_enabled) {
        ndspExit();
        linearFree(audio_buf);
    }
}

void cpymo_backend_audio_lock() 
{
    ndspSetCallback(cpymo_backend_audio_callback_donothing, NULL);
}

void cpymo_backend_audio_unlock()
{
    ndspSetCallback(cpymo_backend_audio_callback, NULL);
}
