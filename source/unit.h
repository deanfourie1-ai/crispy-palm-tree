/*
 * unit.h  –  Unit (character) definitions for the GBA RTS base.
 *
 * Each unit has:
 *   • A tile-based world position (tx, ty)   – 0-based tile coords
 *   • A movement target      (target_tx, target_ty)
 *   • HP / max-HP
 *   • Team  (TEAM_PLAYER or TEAM_ENEMY)
 *   • Type  (UNIT_FOOTMAN, UNIT_PEASANT, UNIT_ORC)
 *   • Active flag
 *
 * OBJ sprite tile indices (see tiles.h):
 *   UNIT_FOOTMAN  → tile 0  (blue)
 *   UNIT_ORC      → tile 1  (green/red)
 *   UNIT_PEASANT  → tile 3  (brown)
 *   Cursor        → tile 2  (yellow)
 *   Selection HL  → tile 4  (yellow)
 */
#pragma once
#include "gba.h"

#define MAX_UNITS 8

typedef enum {
    UNIT_FOOTMAN = 0,
    UNIT_PEASANT,
    UNIT_ORC,
    UNIT_TYPE_COUNT
} UnitType;

#define TEAM_PLAYER 0
#define TEAM_ENEMY  1

typedef struct {
    s16      tx, ty;          /* current tile position */
    s16      target_tx, target_ty; /* movement target */
    u8       hp, max_hp;
    u8       team;
    UnitType type;
    bool     active;
    u8       move_timer;      /* counts down between tile steps */
} Unit;

/* All units; index 0..3 = player, 4..7 = enemy */
extern Unit units[MAX_UNITS];

/* Index of the currently selected player unit (-1 = none) */
extern int  selected_unit;

void unit_init(void);

/*
 * Update unit movement: each unit steps one tile toward its target
 * every UNIT_MOVE_DELAY frames.
 */
void unit_update(void);

/*
 * Write OBJ entries to OAM for all active units plus cursor / highlight.
 *
 * OAM slots used:
 *   0..MAX_UNITS-1   – unit sprites
 *   MAX_UNITS        – cursor sprite  (always drawn)
 *   MAX_UNITS+1      – selection highlight (drawn over selected unit)
 *   rest             – hidden
 */
void unit_draw(s16 cam_x, s16 cam_y, s16 cursor_tx, s16 cursor_ty);

/* Returns unit index at tile (tx, ty), or -1 if none */
int  unit_at(int tx, int ty);
