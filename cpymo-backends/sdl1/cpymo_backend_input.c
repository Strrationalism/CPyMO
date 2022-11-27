#include <cpymo_prelude.h>
#include <cpymo_backend_input.h>
#include <cpymo_engine.h>
#include <string.h>
#include <SDL/SDL_events.h>

#ifdef __WII__
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

extern cpymo_engine engine;
const extern SDL_Surface *framebuffer;

int mouse_wheel = 0;

cpymo_input cpymo_input_snapshot() 
{ 
    cpymo_input x;
    memset(&x, 0, sizeof(x));

    Uint8 *keystate = SDL_GetKeyState(NULL);

    x.left = keystate[SDLK_LEFT];
    x.right = keystate[SDLK_RIGHT];
    x.up = keystate[SDLK_UP];
    x.down = keystate[SDLK_DOWN];

    x.ok = 
        keystate[SDLK_RETURN] ||
        keystate[SDLK_KP_ENTER] || 
        keystate[SDLK_SPACE];

    x.cancel = 
        keystate[SDLK_ESCAPE] || 
        keystate[SDLK_MENU];

    x.hide_window = 
        keystate[SDLK_LSHIFT] |
        keystate[SDLK_RSHIFT];

    x.skip = 
        keystate[SDLK_LCTRL] |
        keystate[SDLK_RCTRL];

    int mouse_x, mouse_y;
    Uint8 mouse_button_state = SDL_GetMouseState(&mouse_x, &mouse_y);

    x.mouse_button = (mouse_button_state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    x.cancel |= (mouse_button_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

    x.mouse_wheel_delta = (int)mouse_wheel;
    mouse_wheel = 0;

    if (framebuffer) {
        x.mouse_position_useable = true;
        x.mouse_x = mouse_x - (framebuffer->w - engine.gameconfig.imagesize_w) / 2;
        x.mouse_y = mouse_y - (framebuffer->h - engine.gameconfig.imagesize_h) / 2;    
    }
    
#ifdef __WII__
    WPAD_ScanPads();
    u32 pressed = WPAD_ButtonsDown(0);

    if (pressed & WPAD_BUTTON_A) x.ok |= true;
    if (pressed & WPAD_BUTTON_B) x.cancel |= true;
    if (pressed & WPAD_BUTTON_UP) x.up |= true;
    if (pressed & WPAD_BUTTON_DOWN) x.down |= true;
    if (pressed & WPAD_BUTTON_LEFT) x.left |= true;
    if (pressed & WPAD_BUTTON_RIGHT) x.right |= true;
    if (pressed & WPAD_BUTTON_1) x.hide_window |= true;
    if (pressed & WPAD_BUTTON_2) x.skip |= true;
#endif

#ifdef __PSP__
    static SDL_Joystick *pspctrl = NULL;
    if (pspctrl == NULL) 
        pspctrl = SDL_JoystickOpen(0);
    
    if (pspctrl) {
        x.cancel |= SDL_JoystickGetButton(pspctrl, 0);
        x.ok |= SDL_JoystickGetButton(pspctrl, 1);
        x.cancel |= SDL_JoystickGetButton(pspctrl, 2);
        x.ok |= SDL_JoystickGetButton(pspctrl, 3);
        x.hide_window |= SDL_JoystickGetButton(pspctrl, 4);
        x.skip |= SDL_JoystickGetButton(pspctrl, 5);

        x.down |= SDL_JoystickGetButton(pspctrl, 6);
        x.up |= SDL_JoystickGetButton(pspctrl, 8);
        x.left |= SDL_JoystickGetButton(pspctrl, 7);
        x.right |= SDL_JoystickGetButton(pspctrl, 9);

        Sint16 cx = SDL_JoystickGetAxis(pspctrl, 0),
               cy = SDL_JoystickGetAxis(pspctrl, 1);

        if (cx >= 32000) x.right = true;
        else if (cx < -32000) x.left = true;

        if (cy > 32000) x.down = true;
        else if (cy < -32000) x.up = true;

        if (SDL_JoystickGetButton(pspctrl, 11))
            cpymo_engine_exit(&engine);
    }
#endif

    

    return x;
}



