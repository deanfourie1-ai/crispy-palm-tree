/* input.c – Button debounce and edge detection */
#include "input.h"

/* Previous and current raw key states (0 = pressed in hardware register) */
static u16 prev_keys = 0xFFFF;
static u16 curr_keys = 0xFFFF;

void input_update(void)
{
    prev_keys = curr_keys;
    curr_keys = REG_KEYINPUT;
}

/* Returns true while the key is held down */
bool input_held(u16 key)
{
    return (curr_keys & key) == 0;
}

/* Returns true only on the first frame the key is pressed */
bool input_pressed(u16 key)
{
    return ((prev_keys & key) != 0) && ((curr_keys & key) == 0);
}

/* Returns true only on the first frame the key is released */
bool input_released(u16 key)
{
    return ((prev_keys & key) == 0) && ((curr_keys & key) != 0);
}
