/*
 * unit.c  –  Stub for Hearthwood Hollow.
 *
 * The original RTS multi-unit / building system has been replaced by the
 * single-hero collecting game in game.c.  This file only satisfies the
 * linker for the symbols declared in unit.h; all functions are no-ops.
 */
#include "unit.h"

Unit units[MAX_UNITS];
int  selected_unit = -1;

void unit_init(void)  {}
void unit_update(void) {}
void unit_update_buildings(void) {}

void unit_draw(s16 cam_x, s16 cam_y, s16 cursor_tx, s16 cursor_ty)
{
    (void)cam_x; (void)cam_y; (void)cursor_tx; (void)cursor_ty;
}

int unit_at(int tx, int ty)
{
    (void)tx; (void)ty;
    return -1;
}

bool unit_building_pos(int btype, s16 *out_tx, s16 *out_ty)
{
    (void)btype; (void)out_tx; (void)out_ty;
    return false;
}

bool unit_building_at(int tx, int ty)
{
    (void)tx; (void)ty;
    return false;
}

bool unit_can_place_building(int btype, int tx, int ty)
{
    (void)btype; (void)tx; (void)ty;
    return false;
}

bool unit_place_building(int btype, s16 tx, s16 ty)
{
    (void)btype; (void)tx; (void)ty;
    return false;
}
