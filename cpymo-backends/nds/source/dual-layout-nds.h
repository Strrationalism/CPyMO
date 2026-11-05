/* NDS 1.2 enhanced dialogue geometry, pure C with no libnds coupling.
 * Derived from upstream 3DS behavior: preserve game-owned message/name
 * box assets while translating their origin toward the top of the touch LCD.
 */
#ifndef CPYMO_NDS_DUAL_LAYOUT_H
#define CPYMO_NDS_DUAL_LAYOUT_H

/* The lower LCD is 192 px tall. Fit artwork in its original scale.
 * The 3DS backend targets a fixed top margin; NDS uses a pixel-space margin,
 * since PyMO logical scene resolutions vary from game to game.
 * Do not move a dialog when that would clip the message or speaker label. */
static inline float nds_dual_dialogue_shift(float logical_height,
    float msg_height,float font_height,float name_offset,
    int has_name,float scale,float letterbox_y){
 if(logical_height<=0.f||msg_height<=0.f||scale<=0.f)return 0.f;
 float origin=logical_height-msg_height;
 if(has_name)origin-=font_height*1.4f+name_offset;
 /* Safe-area constants are physical pixels, not game-specific scale units. */
 const float top_margin=14.f, bottom_limit=185.f;
 const float available=(bottom_limit-top_margin)/scale;
 float content_height=logical_height-origin;
 float target=(top_margin-letterbox_y)/scale;
 float shift=target-origin;
 if(content_height>available){
  /* Oversized original textbox: prefer no shift rather than clipping it
     further. This preserves default legibility until adapted per game. */
  return 0.f;
 }
 float lowest=(bottom_limit-letterbox_y)/scale-logical_height;
 if(shift>lowest)shift=lowest;
 return shift;
}
#endif
