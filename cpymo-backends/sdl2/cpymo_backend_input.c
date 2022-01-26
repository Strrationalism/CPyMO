#include "cpymo_backend_input.h"
#include <SDL.h>
#include <cpymo_engine.h>

extern SDL_Renderer *renderer;
extern SDL_Window *window;
extern cpymo_engine engine;

float mouse_wheel;

cpymo_input cpymo_input_snapshot()
{
	cpymo_input out;

	const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

	out.up = keyboard[SDL_SCANCODE_UP];
	out.down = keyboard[SDL_SCANCODE_DOWN];
	out.left = keyboard[SDL_SCANCODE_LEFT];
	out.right = keyboard[SDL_SCANCODE_RIGHT];
	out.ok = keyboard[SDL_SCANCODE_KP_ENTER] || keyboard[SDL_SCANCODE_RETURN] || keyboard[SDL_SCANCODE_SPACE];
	out.cancel = keyboard[SDL_SCANCODE_ESCAPE] || keyboard[SDL_SCANCODE_CANCEL];
	out.skip = keyboard[SDL_SCANCODE_LCTRL] || keyboard[SDL_SCANCODE_RCTRL];
	out.hide_window = keyboard[SDL_SCANCODE_LSHIFT] || keyboard[SDL_SCANCODE_RSHIFT];
	out.auto_mode = keyboard[SDL_SCANCODE_LALT] || keyboard[SDL_SCANCODE_RALT];

	float scale_x, scale_y;
	SDL_RenderGetScale(renderer, &scale_x, &scale_y);

	int mx, my;
	Uint32 mouse_state = SDL_GetMouseState(&mx, &my);

	out.mouse_button = (mouse_state & SDL_BUTTON_LMASK) != 0;
	out.cancel |= (mouse_state & SDL_BUTTON_RMASK) != 0;
	out.mouse_position_useable = true;

	SDL_Rect viewport;
	SDL_RenderGetViewport(renderer, &viewport);

	out.mouse_x = ((float)mx / scale_x - viewport.x) / (float)viewport.w;
	out.mouse_y = ((float)my / scale_y - viewport.y) / (float)viewport.h;
	out.mouse_wheel_delta = mouse_wheel;

	if (
		out.mouse_x >= 1.0f 
		|| out.mouse_x < 0.0f 
		|| out.mouse_y >= 1.0f 
		|| out.mouse_y < 0.0f 
		|| SDL_GetMouseFocus() != window)
		out.mouse_position_useable = false;

	out.mouse_x *= engine.gameconfig.imagesize_w;
	out.mouse_y *= engine.gameconfig.imagesize_h;

	return out;
}
