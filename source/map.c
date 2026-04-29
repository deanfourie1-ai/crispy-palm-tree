/*
 * map.c  –  Tile-map data and VRAM upload.
 *
 * Hand-crafted 64×32 terrain map:
 *   G = Grass (0)   W = Water (1)   F = Forest (2)
 *   M = Mountain (3)  D = Dirt (4)
 *
 * Layout overview (columns 0-63, rows 0-31):
 *   Row  0-2  : Mountain ridge along the top
 *   Row  3-5  : Forest band below the mountains
 *   Row  6-25 : Open grassland with a dirt road and water features
 *   Row 26-28 : Forest band near the bottom
 *   Row 29-31 : Mountain ridge at the bottom
 */

#include "map.h"

/* SBB 24 → VRAM address 0x0600C000 (first  half of 64×32 = 32×32 tiles) */
/* SBB 25 → VRAM address 0x0600C800 (second half of 64×32 = 32×32 tiles) */
#define MAP_SBB_L  24
#define MAP_SBB_R  25

/* ── Shorthand ────────────────────────────────────────────────────────────  */
#define G TILE_GRASS
#define W TILE_WATER
#define F TILE_FOREST
#define M TILE_MOUNTAIN
#define D TILE_DIRT

/* ── Map data  64 columns × 32 rows ─────────────────────────────────────── */
static const u8 map_data[MAP_H][MAP_W] = {
    /* row  0 */ { M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M },
    /* row  1 */ { M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M },
    /* row  2 */ { M,M,F,F,F,F,M,M,M,M,M,M,F,F,F,F,F,M,M,M,M,M,M,M,M,M,M,F,F,M,M,M,M,M,M,M,M,F,F,F,F,M,M,M,M,M,M,M,M,M,F,F,F,F,M,M,M,M,M,M,M,M,M,M },
    /* row  3 */ { M,F,F,G,G,F,F,M,M,M,M,F,F,G,G,G,F,F,M,M,M,M,M,M,M,F,F,F,G,F,M,M,M,M,M,F,F,F,G,G,F,F,M,M,M,M,M,M,M,F,F,G,G,F,F,M,M,M,M,M,M,M,M,M },
    /* row  4 */ { F,F,G,G,G,G,F,F,M,M,F,F,G,G,G,G,G,F,M,M,M,M,M,F,F,G,G,G,G,F,F,M,M,M,F,F,G,G,G,G,G,F,M,M,M,M,M,M,F,F,G,G,G,F,F,M,M,M,M,M,M,M,M,M },
    /* row  5 */ { F,G,G,G,G,G,G,F,F,F,F,G,G,G,G,G,G,F,F,F,F,F,F,G,G,G,G,G,G,G,F,F,F,F,G,G,G,G,G,G,G,F,F,F,F,F,F,F,G,G,G,G,G,G,F,F,F,F,F,F,F,F,F,F },
    /* row  6 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row  7 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row  8 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row  9 */ { G,G,G,G,G,G,D,D,D,D,D,D,D,D,D,D,D,D,W,W,W,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,G,G,G,G,G,G,G },
    /* row 10 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 11 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 12 */ { G,G,G,G,F,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 13 */ { G,G,F,F,F,G,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 14 */ { G,G,F,G,G,G,G,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 15 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 16 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 17 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 18 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 19 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 20 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 21 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F,G,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 22 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F,G,G,G,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 23 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 24 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 25 */ { G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G },
    /* row 26 */ { F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F },
    /* row 27 */ { F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F },
    /* row 28 */ { M,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F,M },
    /* row 29 */ { M,M,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F,M,M },
    /* row 30 */ { M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M },
    /* row 31 */ { M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M },
};

#undef G
#undef W
#undef F
#undef M
#undef D

/* ── Map query API ───────────────────────────────────────────────────────── */

u8 map_get_tile(int tx, int ty)
{
    if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H)
        return TILE_MOUNTAIN;   /* treat out-of-bounds as impassable */
    return map_data[ty][tx];
}

bool map_tile_blocks(int tx, int ty)
{
    u8 t = map_get_tile(tx, ty);
    return (t == TILE_WATER) || (t == TILE_MOUNTAIN);
}

/* ── VRAM upload ─────────────────────────────────────────────────────────── */
/*
 * The GBA hardware for a 64×32 BG uses two consecutive SBBs:
 *   SBB+0: left  half (columns  0-31, all 32 rows) – 32×32 = 1024 entries
 *   SBB+1: right half (columns 32-63, all 32 rows) – 32×32 = 1024 entries
 * Each entry is a 16-bit tile index.
 */
void map_init(void)
{
    volatile u16 *sbb_l = SBB_BASE(MAP_SBB_L);
    volatile u16 *sbb_r = SBB_BASE(MAP_SBB_R);

    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < 32; tx++) {
            sbb_l[ty * 32 + tx] = map_data[ty][tx];
            sbb_r[ty * 32 + tx] = map_data[ty][tx + 32];
        }
    }
}

/* ── Scroll register update ──────────────────────────────────────────────── */
void map_set_scroll(s16 cam_x, s16 cam_y)
{
    REG_BG0HOFS = (u16)cam_x;
    REG_BG0VOFS = (u16)cam_y;
}
