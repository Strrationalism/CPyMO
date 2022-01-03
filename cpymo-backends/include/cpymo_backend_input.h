#ifndef INCLUDE_CPYMO_BACKEND_INPUT
#define INCLUDE_CPYMO_BACKEND_INPUT

#include <stdbool.h>

typedef struct {
	bool up : 1;
	bool down : 1;
	bool ok : 1;
	bool cancel : 1;
	bool skip : 1;
	bool hide_window : 1;
	bool mouse_button : 1;
	bool mouse_position_useable : 1;
} cpymo_input;

/* Mouse Coord
 *
 * LeftTop - (0, 0)
 * RightBottom - (gameconfig.imagesize.w - 1, gameconfig.imagesize.h - 1)
 *
 * returns true if mouse position is useable.
 * returns false if mouse position is not useable.
 */
cpymo_input cpymo_input_snapshot(float *mouse_x, float *mouse_y);

#endif
