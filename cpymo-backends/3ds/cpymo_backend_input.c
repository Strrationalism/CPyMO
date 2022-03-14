#include <cpymo_backend_input.h>
#include <3ds.h>
#include <stdlib.h>
#include <cpymo_engine.h>

const extern float game_width, game_height;
const extern float offset_xb, offset_yb;
const extern float viewport_width_bottom, viewport_height_bottom;
const extern bool fill_screen;
bool enhanced_3ds_display_mode_touch_ui_enabled(void);
const extern bool enhanced_3ds_display_mode;
extern cpymo_engine engine;

static bool touch_enabled(void) {
    if (!enhanced_3ds_display_mode_touch_ui_enabled()) {
        if (engine.say.active) return true;
        else return false;
    }
    else return true;
}

cpymo_input cpymo_input_snapshot()
{
    cpymo_input out;
    u32 keys = hidKeysHeld();
    
    out.ok = (keys & KEY_A) > 0 || (keys & KEY_Y) > 0 || (keys & KEY_ZL) > 0 || (keys & KEY_ZR) > 0;
    out.skip = (keys & KEY_R) > 0;
    out.cancel = (keys & KEY_B) > 0 || (keys & KEY_X) > 0;
    out.hide_window = (keys & KEY_L) > 0;
    out.auto_mode = (keys & KEY_Y) > 0;
    out.down = (keys & KEY_DOWN) > 0;
    out.up = (keys & KEY_UP) > 0;
    out.left = (keys & KEY_LEFT) > 0;
    out.right = (keys & KEY_RIGHT) > 0;
    out.mouse_button = (keys & KEY_TOUCH) > 0 && enhanced_3ds_display_mode;
    out.mouse_position_useable = out.mouse_button && touch_enabled();
    out.mouse_wheel_delta = 0;

    circlePosition pos;
    hidCircleRead(&pos);

    circlePosition cpos;
    hidCstickRead(&cpos);

    if(pos.dy > 170 || cpos.dy > 32) out.up = true;
    else if(pos.dy < -170 || cpos.dy < -32) out.down = true;

    if(pos.dx > 170 || cpos.dx > 32) out.right = true;
    else if(pos.dx < -170 || cpos.dx < -32) out.left = true;

    if (out.mouse_position_useable) {
        touchPosition touch_pos;
        hidTouchRead(&touch_pos);

        float touch_x = (float)touch_pos.px;
        float touch_y = (float)touch_pos.py;

        if (fill_screen) {
            touch_x /= 320.0f;
            touch_y /= 240.0f;
        }
        else {
            touch_x -= offset_xb;
            touch_y -= offset_yb;
            touch_x /= viewport_width_bottom;
            touch_y /= viewport_height_bottom;
        }

        out.mouse_x = touch_x * game_width;
        out.mouse_y = touch_y * game_height;

        if (touch_x > 1 || touch_x < 0 || touch_y > 1 || touch_y < 0)
            out.mouse_position_useable = false;
    }

    return out;
}
