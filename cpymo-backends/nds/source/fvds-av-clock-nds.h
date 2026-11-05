/* Exact FVDS audio/video clock conversion.
 * Arithmetic only: does not schedule VBlank, ARM7 timers or actual playback.
 */
#ifndef CPYMO_NDS_FVDS_AV_CLOCK_H
#define CPYMO_NDS_FVDS_AV_CLOCK_H
#include <stdint.h>
/* Return the intended number of stereo PCM frames already played at a video
 * frame boundary. Floor division is intentional and deterministic; calculate
 * each boundary independently to prevent accumulated integer drift.
 * All inputs are already checked by nds_fvds_open().
 */
static inline uint64_t nds_fvds_pcm_target(uint32_t frame_boundary,
                                          uint32_t fps_n,uint32_t fps_d,
                                          uint32_t sample_rate){
 if(!fps_n||!fps_d||!sample_rate)return 0;
 return ((uint64_t)frame_boundary*fps_d*sample_rate)/fps_n;
}
/* How many PCM frames to consume in this frame interval? */
static inline uint32_t nds_fvds_pcm_interval(uint32_t frame,
                                             uint32_t fps_n,uint32_t fps_d,
                                             uint32_t sample_rate){
 if(!fps_n||!fps_d||!sample_rate)return 0;
 uint64_t start=nds_fvds_pcm_target(frame,fps_n,fps_d,sample_rate);
 uint64_t stop=nds_fvds_pcm_target(frame+1,fps_n,fps_d,sample_rate);
 return (uint32_t)(stop-start);
}
#endif
