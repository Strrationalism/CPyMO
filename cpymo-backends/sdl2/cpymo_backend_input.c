#include "cpymo_prelude.h"
#include "cpymo_backend_input.h"
#include "cpymo_import_sdl2.h"
#include <cpymo_engine.h>

extern SDL_Renderer *renderer;
extern SDL_Window *window;
extern cpymo_engine engine;

float mouse_wheel;

SDL_GameController **gamecontrollers = NULL;
size_t gamecontrollers_count = 0;

cpymo_input cpymo_input_snapshot()
{
	cpymo_input out;

	const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

	if (keyboard[SDL_SCANCODE_LALT] == 0 &&
		keyboard[SDL_SCANCODE_RALT] == 0)
	{
		out.up = keyboard[SDL_SCANCODE_UP];
		out.down = keyboard[SDL_SCANCODE_DOWN];
		out.left = keyboard[SDL_SCANCODE_LEFT];
		out.right = keyboard[SDL_SCANCODE_RIGHT];
		out.ok = 
			keyboard[SDL_SCANCODE_KP_ENTER] || 
			keyboard[SDL_SCANCODE_RETURN] || 
			keyboard[SDL_SCANCODE_SPACE];
		out.cancel = 
			keyboard[SDL_SCANCODE_ESCAPE] || 
			keyboard[SDL_SCANCODE_CANCEL] || 
			keyboard[SDL_SCANCODE_AC_BACK] ||
			keyboard[SDL_SCANCODE_MENU];
		out.skip = 
			keyboard[SDL_SCANCODE_LCTRL] || 
			keyboard[SDL_SCANCODE_RCTRL];
		out.hide_window = 
			keyboard[SDL_SCANCODE_LSHIFT] || 
			keyboard[SDL_SCANCODE_RSHIFT];
	}
	else {
		out.up = 0;
		out.down = 0;
		out.left = 0;
		out.right = 0;
		out.ok = 0;
		out.cancel = 0;
		out.skip = 0;
		out.hide_window = 0;
	}

#ifdef DISABLE_MOUSE
	out.mouse_position_useable = false;
	out.mouse_button = false;
	out.mouse_wheel_delta = 0;
#else
	float scale_x, scale_y;
	SDL_RenderGetScale(renderer, &scale_x, &scale_y);

    int point_w, point_h, pixel_w, pixel_h;
    SDL_GetWindowSize(window, &point_w, &point_h);
    SDL_GetRendererOutputSize(renderer, &pixel_w, &pixel_h);
    scale_x *= (float) point_w / pixel_w;
    scale_y *= (float) point_h / pixel_h;

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

#endif

	#define MAP_CONTROLLER(OUT_KEY, CONTROLLER_KEY) \
		for (size_t i = 0; i < gamecontrollers_count; ++i) \
			if (SDL_GameControllerGetButton(gamecontrollers[i], CONTROLLER_KEY)) { \
				out.OUT_KEY = true; \
				break; \
			}

	MAP_CONTROLLER(up, SDL_CONTROLLER_BUTTON_DPAD_UP);
	MAP_CONTROLLER(down, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
	MAP_CONTROLLER(left, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
	MAP_CONTROLLER(right, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
	MAP_CONTROLLER(cancel, SDL_CONTROLLER_BUTTON_START);
	MAP_CONTROLLER(cancel, SDL_CONTROLLER_BUTTON_GUIDE);
	MAP_CONTROLLER(cancel, SDL_CONTROLLER_BUTTON_BACK);

#ifdef __PSP__
	MAP_CONTROLLER(ok, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
	MAP_CONTROLLER(ok, SDL_CONTROLLER_BUTTON_LEFTSTICK);
#else
	MAP_CONTROLLER(hide_window, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
	MAP_CONTROLLER(hide_window, SDL_CONTROLLER_BUTTON_LEFTSTICK);
#endif
	
	MAP_CONTROLLER(skip, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
	MAP_CONTROLLER(skip, SDL_CONTROLLER_BUTTON_RIGHTSTICK);

#if defined __SWITCH__ || defined __PSP__
	MAP_CONTROLLER(ok, SDL_CONTROLLER_BUTTON_B);
	MAP_CONTROLLER(ok, SDL_CONTROLLER_BUTTON_X);
	MAP_CONTROLLER(cancel, SDL_CONTROLLER_BUTTON_A);
	MAP_CONTROLLER(cancel, SDL_CONTROLLER_BUTTON_Y);
#else
	MAP_CONTROLLER(ok, SDL_CONTROLLER_BUTTON_A);
	MAP_CONTROLLER(ok, SDL_CONTROLLER_BUTTON_Y);
	MAP_CONTROLLER(cancel, SDL_CONTROLLER_BUTTON_B);
	MAP_CONTROLLER(cancel, SDL_CONTROLLER_BUTTON_X);
#endif

	#undef MAP_CONTROLLER

	for (size_t i = 0; i < gamecontrollers_count; ++i) {
		if (abs(SDL_GameControllerGetAxis(gamecontrollers[i], SDL_CONTROLLER_AXIS_TRIGGERLEFT)) > 16384)
			out.hide_window = true;
		
		if (abs(SDL_GameControllerGetAxis(gamecontrollers[i], SDL_CONTROLLER_AXIS_TRIGGERRIGHT)) > 16384)
			out.skip = true;

		Sint16 x = SDL_GameControllerGetAxis(gamecontrollers[i], SDL_CONTROLLER_AXIS_LEFTX);
		Sint16 y = SDL_GameControllerGetAxis(gamecontrollers[i], SDL_CONTROLLER_AXIS_LEFTY);

		Sint16 x2 = SDL_GameControllerGetAxis(gamecontrollers[i], SDL_CONTROLLER_AXIS_RIGHTX);
		Sint16 y2 = SDL_GameControllerGetAxis(gamecontrollers[i], SDL_CONTROLLER_AXIS_RIGHTY);

		if (abs(x2) > abs(x)) x = x2;
		if (abs(y2) > abs(y)) y = y2;

		if (x > 16384) out.right = true;
		else if (x < -16384) out.left = true;

		if (y > 16384) out.down = true;
		else if (y < -16384) out.up = true;
	}

	return out;
}

void cpymo_input_free_joysticks() 
{
	for (size_t i = 0; i < gamecontrollers_count; ++i)
		SDL_GameControllerClose(gamecontrollers[i]);
	if (gamecontrollers) free(gamecontrollers);
	gamecontrollers = NULL;
	gamecontrollers_count = 0;
}

void cpymo_input_refresh_joysticks() 
{
	cpymo_input_free_joysticks();

	for (int i = 0; i < SDL_NumJoysticks(); ++i) 
		if (SDL_IsGameController(i)) 
			gamecontrollers_count++;

	if (gamecontrollers_count)
		gamecontrollers = (SDL_GameController **)malloc(sizeof(gamecontrollers[0]) * gamecontrollers_count);
	if (gamecontrollers == NULL) gamecontrollers_count = 0;

	if (gamecontrollers) {
		memset(gamecontrollers, 0, sizeof(gamecontrollers[0]) * gamecontrollers_count);
		size_t j = 0;
		for (size_t i = 0; i < gamecontrollers_count; ++i)
			if (SDL_IsGameController((int)i))
				gamecontrollers[j++] = SDL_GameControllerOpen((int)i);
	}
}
