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

static s16 *audio_buf;

static ndspWaveBuf waveBuf[2 * CPYMO_AUDIO_MAX_CHANNELS];

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

static void cpymo_backend_update_volume(size_t cid) 
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

static void cpymo_backend_audio_callback(void *_) 
{
    static unsigned double_buffering = 0;
    if(waveBuf[double_buffering].status != NDSP_WBUF_DONE) return;

    void *dest = waveBuf[double_buffering].data_pcm16;

    size_t size = waveBuf[double_buffering].nsamples * BYTEPERSAMPLE;

    cpymo_audio_copy_mixed_samples(dest, size, &engine.audio);

	DSP_FlushDataCache(dest, size);
    ndspChnWaveBufAdd(0, &waveBuf[double_buffering]);
    double_buffering = !double_buffering;
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

    audio_buf = (s16 *)linearAlloc(buf_size * 2 * CPYMO_AUDIO_MAX_CHANNELS);
    if(audio_buf == NULL) {
        ndspExit();
        printf("[Error] Failed to alloc audio buf.\n");
        return;
    }

    memset(audio_buf, 0, buf_size * 2 * CPYMO_AUDIO_MAX_CHANNELS);
    memset(waveBuf, 0, sizeof(waveBuf));

    for(int cid = 0; cid < CPYMO_AUDIO_MAX_CHANNELS; ++cid) {
        ndspChnReset(cid);
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspChnSetInterp(cid, NDSP_INTERP_LINEAR);
        ndspChnSetRate(cid, SAMPLERATE);
        ndspChnSetFormat(cid, NDSP_FORMAT_STEREO_PCM16);
        cpymo_backend_update_volume(cid);
        
        int bufid = 2 * cid;
        waveBuf[bufid].data_vaddr = ((u8 *)audio_buf) + bufid * buf_size;
        waveBuf[bufid].nsamples = SAMPLESPERBUF;
        ndspChnWaveBufAdd(cid, &waveBuf[bufid]);

        bufid++;
        waveBuf[bufid].data_vaddr = ((u8 *)audio_buf) + bufid * buf_size;
        waveBuf[bufid].nsamples = SAMPLESPERBUF;
        ndspChnWaveBufAdd(cid, &waveBuf[bufid]);
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
