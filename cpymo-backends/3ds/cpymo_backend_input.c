#include <cpymo_backend_input.h>
#include <3ds.h>
#include <stdlib.h>

cpymo_input cpymo_input_snapshot()
{
    cpymo_input out;
    u32 keys = hidKeysHeld();

    if((keys & KEY_ZL)) aptExit();

    out.ok = (keys & KEY_A) > 0;
    out.skip = (keys & KEY_R) > 0;
    out.cancel = (keys & KEY_B) > 0;
    out.hide_window = (keys & KEY_L) > 0;
    out.auto_mode = (keys & KEY_Y) > 0;
    out.down = (keys & KEY_DOWN) > 0;
    out.up = (keys & KEY_UP) > 0;
    out.mouse_button = false;
    out.mouse_position_useable = false;

    circlePosition pos;
    hidCircleRead(&pos);

    if(pos.dy > 170) out.up = true;
    else if(pos.dy < -170) out.down = true;

    return out;
}
