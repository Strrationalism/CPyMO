#include <cpymo_backend_input.h>
#include <3ds.h>
#include <stdlib.h>
#include <cpymo_engine.h>

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

const extern float game_width, game_height;
const extern float ratio_wb, ratio_h;

cpymo_input cpymo_input_snapshot()
{
    cpymo_input out;
    u32 keys = hidKeysHeld();
    
    out.ok = (keys & KEY_A) > 0 || (keys & KEY_Y) > 0 || (keys & KEY_ZL) > 0 || (keys & KEY_ZR) > 0;
    out.skip = (keys & KEY_R) > 0;
    out.cancel = (keys & KEY_B) > 0 || (keys & KEY_X) > 0;
    out.hide_window = (keys & KEY_L) > 0;
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

    if(pos.dy > 170 || cpos.dy > 64) out.up = true;
    else if(pos.dy < -170 || cpos.dy < -64) out.down = true;

    if(pos.dx > 170 || cpos.dx > 64) out.right = true;
    else if(pos.dx < -170 || cpos.dx < -64) out.left = true;

    if (out.mouse_position_useable) {
        touchPosition touch_pos;
        hidTouchRead(&touch_pos);

        const float r = ratio_wb > ratio_h ? ratio_wb : ratio_h;
        
        const float
            viewport_w = game_width / r,
            viewport_h = game_height / r;

        const float
            offset_x = (320 - viewport_w) / 2.0f,
            offset_y = (240 - viewport_h) / 2.0f;

        float touch_x = (float)touch_pos.px;
        float touch_y = (float)touch_pos.py;

        touch_x -= offset_x;
        touch_y -= offset_y;
        touch_x /= viewport_w;
        touch_y /= viewport_h;

        out.mouse_x = touch_x * game_width;
        out.mouse_y = touch_y * game_height;

        if (touch_x > 1 || touch_x < 0 || touch_y > 1 || touch_y < 0)
            out.mouse_position_useable = false;
    }

    return out;
}
