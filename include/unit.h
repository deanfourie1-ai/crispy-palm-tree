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

/* Gather resource type (stored in Unit.gather_type) */
#define GATHER_WOOD  0
#define GATHER_FISH  1
#define GATHER_ORE   2

typedef enum {
    PEASANT_IDLE = 0,
    PEASANT_MINIGAME,    /* overlay mini-game running before gathering */
    PEASANT_GATHERING,
    PEASANT_RETURNING
} PeasantState;

typedef struct {
    s16         tx, ty;          /* current tile position */
    s16         target_tx, target_ty; /* movement target */
    u8          hp, max_hp;
    u8          team;
    UnitType    type;
    bool        active;
    u8          move_timer;      /* counts down between tile steps */
    u8          gather_timer;    /* counts up while gathering */
    PeasantState pstate;         /* gather state machine (peasants only) */
    u8          carrying;        /* wood carried, not yet deposited */
    bool        auto_gather;     /* keep looping gather/return for a cluster */
    s16         gather_cluster;  /* initial forest cluster id (-1 if none) */
    u8          gather_type;     /* GATHER_WOOD / GATHER_FISH / GATHER_ORE */
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

/* Advance building construction timers and visual build levels. */
void unit_update_buildings(void);

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

/* Returns true and sets out_tx/out_ty to the building tile of 'btype' */
bool unit_building_pos(int btype, s16 *out_tx, s16 *out_ty);

/* Returns true if any active building occupies tile (tx, ty). */
bool unit_building_at(int tx, int ty);

/* Returns true when a building type can be placed at tile (tx, ty). */
bool unit_can_place_building(int btype, int tx, int ty);

/* Place (or move) a building type to tile (tx, ty). */
bool unit_place_building(int btype, s16 tx, s16 ty);
