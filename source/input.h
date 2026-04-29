/* input.h – Button state query API */
#pragma once
#include "gba.h"

/*
 * Call input_update() once at the top of every game frame (after VBlank).
 * Then query button state with the helpers below.
 *
 * Held   – button is currently pressed
 * Pressed – button just went down this frame (rising edge)
 * Released – button just went up this frame (falling edge)
 */

void input_update(void);

bool input_held(u16 key);
bool input_pressed(u16 key);
bool input_released(u16 key);
