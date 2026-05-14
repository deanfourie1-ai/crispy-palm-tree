/*
 * results.h  –  Day-end results screen for Hearthwood Hollow.
 *
 * Rendered in GBA Mode 3 (240×160 16-bit bitmap).
 * Shows the day's harvest totals and a cozy streak meter.
 * Press A to advance to the next day.
 */
#pragma once
#include "gba.h"

/* Initialise and draw the results screen for the completed day.
 * day        – day number just completed (1-based)
 * wood_today – wood logs gathered this day
 * gold_today – gold ore gathered this day
 * fish_today – river fish gathered this day
 * streak     – current consecutive-day streak (1-7)
 */
void results_init(int day, int wood_today, int gold_today, int fish_today, int streak);

/* Process one frame.  Returns true when the player has pressed A and the
 * screen should advance to the next day. */
bool results_update(void);
