/*
 * main.c  –  Hearthwood Hollow entry point.
 *
 * Game phases:
 *   PHASE_TITLE   – Mode 3 title screen (menu.c)
 *   PHASE_GAME    – Mode 0 tile overworld + Mode 3 mini-game overlays (game.c)
 *   PHASE_RESULTS – Mode 3 day-end results screen (results.c)
 *
 * Flow:
 *   Title → PRESS START → Game (day N)
 *   Game  → day over    → Results
 *   Results → PRESS A   → Game (day N+1)  [hero state persists, today-totals reset]
 */

#include "gba.h"
#include "video.h"
#include "input.h"
#include "map.h"
#include "unit.h"
#include "game.h"
#include "menu.h"
#include "minigame.h"
#include "results.h"

typedef enum {
    PHASE_TITLE = 0,
    PHASE_GAME,
    PHASE_RESULTS
} GamePhase;

int main(void)
{
    int streak = 0;

    while (true) {
        /* ── Title screen ───────────────────────────────────────────────── */
        menu_init();

        while (true) {
            vblank_wait();
            input_update();
            if (menu_update() == MENU_ACTION_NEW_GAME)
                break;
        }

        /* Reset persistent state for a fresh run */
        wood = 0; food = 0; gold = 0;
        game_day     = 1;
        hero_hearts  = 3;
        hovel_level  = 0;
        streak       = 0;

        /* ── Day loop ──────────────────────────────────────────────────── */
        while (true) {
            /* ── Overworld / mini-game phase ─────────────────────────── */
            video_init();
            map_set_active((MapId)0);
            map_init();
            /* Hide all OAM on entry */
            for (int i = 0; i < 128; i++) {
                OAM_MEM[i].attr0 = OBJ_HIDE;
                OAM_MEM[i].attr1 = 0;
                OAM_MEM[i].attr2 = 0;
            }
            /* Reset today totals */
            wood_today = 0;
            gold_today = 0;
            fish_today = 0;
            day_gathers_left = DAY_GOAL_TOTAL;
            game_init();

            while (true) {
                vblank_wait();
                input_update();
                game_update();

                if (!minigame_is_active()) {
                    REG_DISPCNT |= (DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D);
                    game_hud_draw();
                }

                if (game_day_over())
                    break;
            }

            /* ── Results screen ──────────────────────────────────────── */
            /* Gather events this day: did they collect something? */
            if (wood_today > 0 || gold_today > 0 || fish_today > 0)
                streak++;
            else
                streak = 0;
            if (streak > 7) streak = 7;

            results_init(game_day, wood_today, gold_today, fish_today, streak);

            while (true) {
                vblank_wait();
                input_update();
                if (results_update())
                    break;
            }

            /* Advance to next day */
            game_day++;

            /* If the player somehow died (0 hearts), go back to title */
            if (hero_hearts <= 0)
                break;
        }
        /* Loop back to title */
    }

    return 0;
}
