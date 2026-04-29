/*
 * game.h  –  RTS game-state and per-frame update logic.
 *
 * Controls:
 *   D-pad        Move the cursor around the map
 *   A            Select a player unit at the cursor, or issue a move
 *                command to the cursor tile if a unit is already selected
 *   B            Deselect current unit / cancel
 *   Start        (reserved for pause / menu)
 *   L / R        Scroll camera independently of cursor (future)
 */
#pragma once
#include "gba.h"

/* Camera position in pixels (top-left corner of the visible area) */
extern s16 cam_x, cam_y;

/* Cursor tile position */
extern s16 cursor_tx, cursor_ty;

/* Initialise game state (call after video_init / map_init / unit_init) */
void game_init(void);

/* Process one frame of input and game logic */
void game_update(void);
