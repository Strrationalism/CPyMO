/* NDS 1.2 physical touch routing, pure C for ROM + host regression.
 * A display mode is not itself proof of a visible interactive touch
 * layer: enhanced mode's hidden lower LCD cannot accept menu taps.
 */
#ifndef CPYMO_NDS_DUAL_INPUT_H
#define CPYMO_NDS_DUAL_INPUT_H
typedef struct {
 float x,y;
 unsigned mouse_button,ok;
} nds_touch_sample;

/* 0 classic upper: touch bottom auxiliary to advance.
 * 1 classic lower: native game touch coordinates.
 * 2 enhanced: native coordinate choice/UI hit-tests, narrative tap=A.
 * hidden enhanced lower: first touch is separately consumed as wake-up. */
static inline unsigned nds_touch_uses_coordinates(unsigned mode,
  unsigned enhanced_visible,unsigned ui,unsigned choice){
 return mode==1 || (mode==2 && enhanced_visible && (ui||choice));
}
/* A physical pen contact must keep the meaning it had on touch-down.
 * PyMO can enter a script choice or menu while that same contact is held;
 * enhanced narrative touch=A would otherwise become menu mouse=pressed.
 * Conversely a menu->story transition could synthesize an A release.
 * Return 0 (no boundary), 1 (sanitize previous input), or 2 (sanitize
 * previous input and block the continuing pen until it is lifted).
 * A fresh pen-down AFTER a UI transition is always accepted normally. */
typedef struct {
 unsigned valid,coordinate_role,physical_held;
} nds_touch_role_tracker;

static inline unsigned nds_touch_role_step(nds_touch_role_tracker* state,
                                           unsigned coordinate_role,
                                           unsigned physical_held){
 if(!state)return 0;
 coordinate_role=!!coordinate_role;
 physical_held=!!physical_held;
 unsigned crossing=state->valid && state->coordinate_role!=coordinate_role
                   && state->physical_held;
 state->valid=1;
 state->coordinate_role=coordinate_role;
 state->physical_held=physical_held;
 return crossing?(physical_held?2u:1u):0u;
}

static inline nds_touch_sample nds_touch_project(unsigned mode,
 unsigned enhanced_visible,unsigned ui,unsigned choice,
 unsigned physical_touch,unsigned blocked,unsigned key_a,
 int screen_x,int screen_y,float screen_scale,float letterbox_x,
 float letterbox_y){
 nds_touch_sample out={0,0,0,0};
 unsigned touch=physical_touch&&!blocked;
 unsigned coordinates=nds_touch_uses_coordinates(mode,enhanced_visible,ui,choice);
 out.ok=!!key_a || (touch&&!coordinates);
 out.mouse_button=touch&&coordinates;
 if(out.mouse_button && screen_scale>0.f){
  out.x=(screen_x-letterbox_x)/screen_scale;
  out.y=(screen_y-letterbox_y)/screen_scale;
 }
 return out;
}
#endif
