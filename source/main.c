/*
 * main.c  –  GBA RTS base game entry point.
 *
 * Game concept: simplified Warcraft-like RTS on the GBA.
 *
 * Features in this base:
 *   • Scrollable 64×32-tile terrain map (grass, water, forest, mountain, dirt)
 *   • Player units: 3 Footmen + 1 Peasant  (blue sprites)
 *   • Enemy  units: 3 Orcs                 (green sprites)
 *   • Cursor driven by the D-pad; camera auto-follows with margin scrolling
 *   • A  → select player unit / issue move command / attack adjacent enemy
 *   • B  → deselect / cancel
 */

#include "gba.h"
#include "video.h"
#include "input.h"
#include "map.h"
#include "unit.h"
#include "game.h"

int main(void)
{
    video_init();
    map_init();
    unit_init();
    game_init();

    while (true) {
        /* Wait for vertical-blank so all writes land between frames */
        vblank_wait();

        /* Sample buttons */
        input_update();

        /* Run one tick of game logic (input, movement, camera, BG scroll) */
        game_update();

        /* Write sprite data to OAM */
        unit_draw(cam_x, cam_y, cursor_tx, cursor_ty);
    }

    return 0;
}
