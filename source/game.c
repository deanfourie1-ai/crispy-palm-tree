/*
 * game.c  –  RTS game logic: input handling, cursor movement, unit commands.
 */
#include "game.h"
#include "input.h"
#include "map.h"
#include "unit.h"

/* ── State ─────────────────────────────────────────────────────────────────  */
s16 cam_x    = 0;
s16 cam_y    = 0;
s16 cursor_tx = 4;   /* cursor starts near player units */
s16 cursor_ty = 24;

/* How many tiles from screen edge trigger a camera scroll */
#define SCROLL_MARGIN_T 3    /* tiles from edge before camera follows */

/* ── Init ──────────────────────────────────────────────────────────────────  */
void game_init(void)
{
    cam_x    = 0;
    cam_y    = 0;
    cursor_tx = 4;
    cursor_ty = 24;
}

/* ── Per-frame update ──────────────────────────────────────────────────────  */
void game_update(void)
{
    /* ── Cursor movement ────────────────────────────────────────────────── */
    if (input_pressed(KEY_RIGHT) && cursor_tx < MAP_W - 1) cursor_tx++;
    if (input_pressed(KEY_LEFT)  && cursor_tx > 0         ) cursor_tx--;
    if (input_pressed(KEY_DOWN)  && cursor_ty < MAP_H - 1 ) cursor_ty++;
    if (input_pressed(KEY_UP)    && cursor_ty > 0         ) cursor_ty--;

    /* ── Camera follows cursor with a margin ────────────────────────────── */
    /* Convert cursor tile to pixel, then clamp camera so cursor stays on screen */
    int cx_px = cursor_tx * 8;
    int cy_px = cursor_ty * 8;

    /* Scroll right if cursor gets close to right edge */
    if (cx_px - cam_x > SCREEN_W - SCROLL_MARGIN_T * 8)
        cam_x = (s16)(cx_px - (SCREEN_W - SCROLL_MARGIN_T * 8));
    /* Scroll left */
    if (cx_px - cam_x < SCROLL_MARGIN_T * 8)
        cam_x = (s16)(cx_px - SCROLL_MARGIN_T * 8);
    /* Scroll down */
    if (cy_px - cam_y > SCREEN_H - SCROLL_MARGIN_T * 8)
        cam_y = (s16)(cy_px - (SCREEN_H - SCROLL_MARGIN_T * 8));
    /* Scroll up */
    if (cy_px - cam_y < SCROLL_MARGIN_T * 8)
        cam_y = (s16)(cy_px - SCROLL_MARGIN_T * 8);

    /* Clamp camera to map bounds */
    if (cam_x < 0)           cam_x = 0;
    if (cam_x > CAM_MAX_X)   cam_x = (s16)CAM_MAX_X;
    if (cam_y < 0)           cam_y = 0;
    if (cam_y > CAM_MAX_Y)   cam_y = (s16)CAM_MAX_Y;

    /* ── A button: select / command ─────────────────────────────────────── */
    if (input_pressed(KEY_A)) {
        int hit = unit_at(cursor_tx, cursor_ty);

        if (selected_unit >= 0) {
            /* A unit is already selected */
            Unit *su = &units[selected_unit];

            if (hit == selected_unit) {
                /* Pressed A on the same unit again → deselect */
                selected_unit = -1;
            } else if (hit >= 0 && units[hit].team == TEAM_PLAYER) {
                /* Pressed A on a different player unit → switch selection */
                selected_unit = hit;
            } else if (hit >= 0 && units[hit].team == TEAM_ENEMY) {
                /* Pressed A on an enemy → simple attack (reduce HP) */
                Unit *eu = &units[hit];
                if (eu->hp > 0) eu->hp--;
                if (eu->hp == 0) eu->active = false;
            } else {
                /* Pressed A on empty tile → issue move command */
                su->target_tx = cursor_tx;
                su->target_ty = cursor_ty;
            }
        } else {
            /* Nothing selected: try to select a player unit at cursor */
            if (hit >= 0 && units[hit].team == TEAM_PLAYER) {
                selected_unit = hit;
            }
        }
    }

    /* ── B button: deselect ─────────────────────────────────────────────── */
    if (input_pressed(KEY_B)) {
        selected_unit = -1;
    }

    /* ── Update unit AI / movement ──────────────────────────────────────── */
    unit_update();

    /* ── Update BG scroll registers ─────────────────────────────────────── */
    map_set_scroll(cam_x, cam_y);
}
