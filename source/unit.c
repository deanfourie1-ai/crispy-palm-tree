/* unit.c  –  Unit data, movement and sprite rendering */
#include "unit.h"
#include "map.h"

/* ── Constants ────────────────────────────────────────────────────────────── */
#define UNIT_MOVE_DELAY  20    /* frames between tile steps */

/* OBJ tile indices (must match obj_tiles[] order in tiles.h) */
#define SPR_FOOTMAN   0
#define SPR_ORC       1
#define SPR_CURSOR    2
#define SPR_PEASANT   3
#define SPR_HIGHLIGHT 4

/* ── Global state ─────────────────────────────────────────────────────────── */
Unit units[MAX_UNITS];
int  selected_unit = -1;

/* ── Initialisation ───────────────────────────────────────────────────────── */
void unit_init(void)
{
    /* Player Footman #1 – starts bottom-left area */
    units[0] = (Unit){ .tx=4,  .ty=24, .target_tx=4,  .target_ty=24,
                       .hp=5,  .max_hp=5, .team=TEAM_PLAYER,
                       .type=UNIT_FOOTMAN, .active=true };
    /* Player Footman #2 */
    units[1] = (Unit){ .tx=6,  .ty=24, .target_tx=6,  .target_ty=24,
                       .hp=5,  .max_hp=5, .team=TEAM_PLAYER,
                       .type=UNIT_FOOTMAN, .active=true };
    /* Player Peasant */
    units[2] = (Unit){ .tx=5,  .ty=26, .target_tx=5,  .target_ty=26,
                       .hp=3,  .max_hp=3, .team=TEAM_PLAYER,
                       .type=UNIT_PEASANT, .active=true };
    /* Player Footman #3 */
    units[3] = (Unit){ .tx=7,  .ty=25, .target_tx=7,  .target_ty=25,
                       .hp=5,  .max_hp=5, .team=TEAM_PLAYER,
                       .type=UNIT_FOOTMAN, .active=true };

    /* Enemy Orc #1 – starts top-right area */
    units[4] = (Unit){ .tx=56, .ty=6,  .target_tx=56, .target_ty=6,
                       .hp=6,  .max_hp=6, .team=TEAM_ENEMY,
                       .type=UNIT_ORC, .active=true };
    /* Enemy Orc #2 */
    units[5] = (Unit){ .tx=58, .ty=6,  .target_tx=58, .target_ty=6,
                       .hp=6,  .max_hp=6, .team=TEAM_ENEMY,
                       .type=UNIT_ORC, .active=true };
    /* Enemy Orc #3 */
    units[6] = (Unit){ .tx=57, .ty=8,  .target_tx=57, .target_ty=8,
                       .hp=6,  .max_hp=6, .team=TEAM_ENEMY,
                       .type=UNIT_ORC, .active=true };
    /* Slot 7 unused */
    units[7].active = false;
}

/* ── Tile-step movement ───────────────────────────────────────────────────── */
void unit_update(void)
{
    for (int i = 0; i < MAX_UNITS; i++) {
        Unit *u = &units[i];
        if (!u->active) continue;
        if (u->tx == u->target_tx && u->ty == u->target_ty) continue;

        if (u->move_timer > 0) {
            u->move_timer--;
            continue;
        }
        u->move_timer = UNIT_MOVE_DELAY;

        /* Step one tile toward target (X then Y) */
        int dx = 0, dy = 0;
        if      (u->target_tx > u->tx) dx = 1;
        else if (u->target_tx < u->tx) dx = -1;
        else if (u->target_ty > u->ty) dy = 1;
        else if (u->target_ty < u->ty) dy = -1;

        int nx = u->tx + dx;
        int ny = u->ty + dy;

        /* Skip step if tile is impassable or occupied */
        if (map_tile_blocks(nx, ny)) {
            /* Give up – reset target to current position */
            u->target_tx = u->tx;
            u->target_ty = u->ty;
            continue;
        }
        if (unit_at(nx, ny) >= 0) {
            /* Occupied – try the perpendicular direction */
            nx = u->tx;
            ny = u->ty;
            if (dx != 0) {
                /* Try Y step instead */
                if      (u->target_ty > u->ty) ny = u->ty + 1;
                else if (u->target_ty < u->ty) ny = u->ty - 1;
            } else {
                if      (u->target_tx > u->tx) nx = u->tx + 1;
                else if (u->target_tx < u->tx) nx = u->tx - 1;
            }
            if (map_tile_blocks(nx, ny) || unit_at(nx, ny) >= 0) {
                u->target_tx = u->tx;
                u->target_ty = u->ty;
                continue;
            }
        }

        u->tx = (s16)nx;
        u->ty = (s16)ny;
    }
}

/* ── Helper: find unit at tile ────────────────────────────────────────────── */
int unit_at(int tx, int ty)
{
    for (int i = 0; i < MAX_UNITS; i++) {
        if (units[i].active && units[i].tx == tx && units[i].ty == ty)
            return i;
    }
    return -1;
}

/* ── OAM sprite type lookup ────────────────────────────────────────────────── */
static u8 unit_sprite_tile(UnitType type)
{
    switch (type) {
        case UNIT_FOOTMAN: return SPR_FOOTMAN;
        case UNIT_PEASANT: return SPR_PEASANT;
        case UNIT_ORC:     return SPR_ORC;
        default:           return SPR_FOOTMAN;
    }
}

/* ── Write OAM ────────────────────────────────────────────────────────────── */

/* Helper: hide a single OAM slot */
static void oam_hide(int slot)
{
    volatile OBJ_ATTR *oam = OAM_MEM + slot;
    oam->attr0 = OBJ_HIDE;
    oam->attr1 = 0;
    oam->attr2 = 0;
}

void unit_draw(s16 cam_x, s16 cam_y, s16 cursor_tx, s16 cursor_ty)
{
    volatile OBJ_ATTR *oam = OAM_MEM;

    /* ── Unit sprites ─────────────────────────────────────────────────── */
    for (int i = 0; i < MAX_UNITS; i++) {
        Unit *u = &units[i];
        if (!u->active) {
            oam_hide(i);
            continue;
        }

        int sx = u->tx * 8 - cam_x;
        int sy = u->ty * 8 - cam_y;

        /* Only draw if on screen (with a 1-tile margin for partial sprites) */
        if (sx < -8 || sx >= SCREEN_W || sy < -8 || sy >= SCREEN_H) {
            oam_hide(i);
            continue;
        }

        u8 tile = unit_sprite_tile(u->type);
        oam[i].attr0 = OBJ_Y(sy) | OBJ_SHAPE_SQ | OBJ_4BPP;
        oam[i].attr1 = OBJ_X(sx) | OBJ_SIZE_8;
        oam[i].attr2 = OBJ_TILE(tile) | OBJ_PRIO(1) | OBJ_PAL(0);
    }

    /* ── Cursor sprite ─────────────────────────────────────────────────── */
    {
        int slot = MAX_UNITS;
        int sx   = cursor_tx * 8 - cam_x;
        int sy   = cursor_ty * 8 - cam_y;

        if (sx >= 0 && sx < SCREEN_W && sy >= 0 && sy < SCREEN_H) {
            oam[slot].attr0 = OBJ_Y(sy) | OBJ_SHAPE_SQ | OBJ_4BPP;
            oam[slot].attr1 = OBJ_X(sx) | OBJ_SIZE_8;
            oam[slot].attr2 = OBJ_TILE(SPR_CURSOR) | OBJ_PRIO(0) | OBJ_PAL(0);
        } else {
            oam_hide(slot);
        }
    }

    /* ── Selection highlight ───────────────────────────────────────────── */
    {
        int slot = MAX_UNITS + 1;
        if (selected_unit >= 0 && units[selected_unit].active) {
            Unit *su = &units[selected_unit];
            int sx   = su->tx * 8 - cam_x;
            int sy   = su->ty * 8 - cam_y;
            if (sx >= 0 && sx < SCREEN_W && sy >= 0 && sy < SCREEN_H) {
                oam[slot].attr0 = OBJ_Y(sy) | OBJ_SHAPE_SQ | OBJ_4BPP;
                oam[slot].attr1 = OBJ_X(sx) | OBJ_SIZE_8;
                oam[slot].attr2 = OBJ_TILE(SPR_HIGHLIGHT) | OBJ_PRIO(0) | OBJ_PAL(0);
            } else {
                oam_hide(slot);
            }
        } else {
            oam_hide(slot);
        }
    }

    /* Hide any remaining OAM slots */
    for (int i = MAX_UNITS + 2; i < 128; i++) {
        oam_hide(i);
    }
}
