#include <cpymo_backend_input.h>
#include <cpymo_engine.h>
#include <string.h>
#include <SDL_events.h>

const extern cpymo_engine engine;
const extern SDL_Surface *framebuffer;

cpymo_input cpymo_input_snapshot() 
{ 
    cpymo_input i;
    memset(&i, 0, sizeof(i));

    Uint8 *keystate = SDL_GetKeyState(NULL);

    i.left = keystate[SDLK_LEFT];
    i.right = keystate[SDLK_RIGHT];
    i.up = keystate[SDLK_UP];
    i.down = keystate[SDLK_DOWN];

    i.ok = 
        keystate[SDLK_RETURN] | 
        keystate[SDLK_KP_ENTER] | 
        keystate[SDLK_SPACE];

    i.cancel = keystate[SDLK_ESCAPE];

    i.hide_window = 
        keystate[SDLK_LSHIFT] |
        keystate[SDLK_RSHIFT];

    i.skip = 
        keystate[SDLK_LCTRL] |
        keystate[SDLK_RCTRL];

    int mouse_x, mouse_y;
    Uint8 mouse_button_state = SDL_GetMouseState(&mouse_x, &mouse_y);

    i.mouse_button = (mouse_button_state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    i.cancel |= (mouse_button_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

    if (mouse_button_state & SDL_BUTTON(SDL_BUTTON_WHEELUP)) 
        i.mouse_wheel_delta = -1;
    if (mouse_button_state & SDL_BUTTON(SDL_BUTTON_WHEELDOWN))
        i.mouse_wheel_delta = 1;

    if (framebuffer) {
        i.mouse_position_useable = true;
        i.mouse_x = mouse_x - (framebuffer->w - engine.gameconfig.imagesize_w) / 2;
        i.mouse_y = mouse_y - (framebuffer->h - engine.gameconfig.imagesize_h) / 2;    
    }

    return i;
}


