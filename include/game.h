/*
 * game.h  –  Hearthwood Hollow game state and per-frame update.
 *
 * Single-hero collecting game.
 * Willow (the hero) walks around the map gathering Wood, Gold and Fish.
 * Each resource triggers a themed mini-game when reached.
 * After DAY_GOAL_TOTAL resources are gathered, the day ends and the results
 * screen is shown.
 *
 * Controls:
 *   D-pad   Move Willow around the map
 *   A       Gather the resource nearest to Willow / confirm
 *   B       (reserved / cancel)
 *   Start   End the day early (jump to results)
 */
#pragma once
#include "gba.h"

/* Camera position in pixels (top-left corner of the visible area) */
extern s16 cam_x, cam_y;

/* Hero tile position */
extern s16 cursor_tx, cursor_ty;

/* Lifetime totals */
extern int wood;
extern int food;
extern int gold;

/* Day-scope totals (reset each day) */
extern int wood_today;
extern int gold_today;
extern int fish_today;

/* Current day number (1-based) */
extern int game_day;

/* Hero hearts (0-4, starts at 3) */
extern int hero_hearts;
#define HERO_MAX_HEARTS 4

/* Hovel upgrade level (0-4, persists across days, resets on new game) */
extern int hovel_level;
#define HOVEL_MAX_LEVEL 4

/* Number of gather events remaining this day before it ends */
#define DAY_GOAL_TOTAL  6   /* gather 6 times → end of day */
extern int day_gathers_left;

/* Returns true when the day is over and the results screen should show */
bool game_day_over(void);

/* Initialise game state (call after video_init / map_init) */
void game_init(void);

/* Process one frame of input and game logic */
void game_update(void);

/* Draw HUD sprites (resource counters, hearts, day label) */
void game_hud_draw(void);
