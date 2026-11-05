/* 1.2 enhanced-display opening-screen gate.
 * Keep the original title/entry selection as one authored upper-screen
 * scene, instead of splitting its decorative labels and input sprites.
 * Transition only on actual narrative dialogue, not on a script filename
 * (PyMO games use many different startup script names). Independently
 * tracks narrative progress even when the player starts in classic mode.
 * The gate is session-local; it never touches saves or script state.
 * Public independent implementation, header also compiled in host tests.
 */
#ifndef CPYMO_NDS_DUAL_TITLE_H
#define CPYMO_NDS_DUAL_TITLE_H

typedef struct {
 unsigned before_story;
} nds_dual_title_gate;

static inline void nds_dual_title_reset(nds_dual_title_gate* gate){
 gate->before_story=1u;
}
static inline unsigned nds_dual_title_observe(nds_dual_title_gate* gate,
                                               unsigned say_active,
                                               unsigned ui_active,
                                               unsigned choice_active){
 if(!gate->before_story || !say_active || ui_active || choice_active)
  return 0u;
 gate->before_story=0u;
 return 1u; /* one-time transition: repaint lower LCD as dialogue */
}
static inline unsigned nds_dual_title_allow_split(const nds_dual_title_gate* gate){
 return !gate->before_story;
}
#endif
