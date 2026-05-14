/*
 * minigame.h  –  Gathering mini-game overlay system.
 *
 * Triggers a "battle initiate" style overlay when a peasant arrives at a
 * resource tile.  The overlay uses GBA Mode 3 (240×160 bitmap) over the
 * Mode 0 game world, transitions in/out with a curtain wipe animation, and
 * returns a yield value to the calling unit state machine.
 *
 * Typical call sequence (from game.c peasant state machine):
 *
 *   // On arrival at resource:
 *   if (!minigame_is_active()) {
 *       minigame_start(u->gather_type);
 *       u->pstate = PEASANT_MINIGAME;
 *   } else {
 *       u->pstate       = PEASANT_GATHERING;   // fallback: another in progress
 *       u->gather_timer = 0;
 *   }
 *
 *   // Each frame while in PEASANT_MINIGAME:
 *   minigame_update();
 *   if (minigame_is_done()) {
 *       u->carrying = minigame_get_yield();
 *       minigame_reset();
 *       // … set PEASANT_RETURNING …
 *   }
 */
#pragma once
#include "gba.h"

/*
 * minigame_start  –  Switch to Mode 3, draw themed backdrop, begin
 *                    transition-in animation.
 *
 * gather_type: GATHER_WOOD / GATHER_FISH / GATHER_ORE  (from unit.h)
 */
void minigame_start(u8 gather_type);

/*
 * minigame_update  –  Advance one frame of overlay logic.
 *                     Call exactly once per frame while is_active() is true.
 *                     Handles transitions, input, and the active mini-game.
 */
void minigame_update(void);

/*
 * minigame_is_active  –  Returns true between start() and reset().
 *                        Use this to suppress unit_draw / game_hud_draw in
 *                        main.c while the overlay is visible.
 */
bool minigame_is_active(void);

/*
 * minigame_is_done  –  Returns true once the transition-out animation has
 *                      finished and the yield is ready to read.
 */
bool minigame_is_done(void);

/*
 * minigame_get_yield  –  Returns the resource amount earned.
 *                         Call once after is_done(); result is based on the
 *                         player's performance during the active phase.
 *                         Scores map to:
 *                           0 = miss  → base yield
 *                           1 = ok    → base yield
 *                           2 = good  → base + 2
 *                           3 = great → base + 5
 *                           4 = perfect → base + 10
 */
u8 minigame_get_yield(void);

/*
 * minigame_reset  –  Return state to INACTIVE so the next arrival can
 *                    trigger a fresh mini-game.  Call after reading yield.
 */
void minigame_reset(void);
