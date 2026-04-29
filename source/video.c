/*
 * video.c  –  Display initialisation.
 *
 * Configures:
 *   • DISPCNT: Mode 0 (tile BG), BG0 enabled, OBJ enabled (1-D mapping)
 *   • BG0CNT:  CBB 0 (tile graphics), SBB 24 (map), 4-bpp, 64×32 tiles
 *   • Uploads BG and OBJ palettes to Palette RAM
 *   • Uploads BG tile graphics to VRAM CBB 0
 *   • Uploads OBJ sprite tiles to OBJ tile memory (VRAM + 64 KB)
 */

#include "video.h"
#include "tiles.h"

/* ── BG layout constants ────────────────────────────────────────────────── */
/* CBB 0  → VRAM 0x06000000 (BG tile graphics)                              */
/* SBB 24 → VRAM 0x0600C000 (BG0 tile map, first 2 KB of a 64×32 map)       */
/* SBB 25 → VRAM 0x0600C800 (BG0 tile map, second 2 KB – right half)        */
#define MAP_CBB  0
#define MAP_SBB  24

void video_init(void)
{
    /* ── Display control ────────────────────────────────────────────────── */
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_OBJ | DCNT_OBJ_1D;

    /* ── BG0 control: CBB 0, SBB 24, 4-bpp, 64×32 tiles ───────────────── */
    REG_BG0CNT  = BG_CBB(MAP_CBB) | BG_SBB(MAP_SBB) | BG_4BPP
                | BG_SIZE_64x32 | BG_PRIO(3);

    /* ── BG palette ──────────────────────────────────────────────────────  */
    memcpy16(MEM_PAL_BG, bg_palette, 16);

    /* ── OBJ palette ─────────────────────────────────────────────────────  */
    memcpy16(MEM_PAL_OBJ, obj_palette, 16);

    /* ── BG tile graphics → CBB 0 ───────────────────────────────────────  */
    /* Each tile is 32 bytes; 5 tiles total                                 */
    volatile u8 *cbb0 = (volatile u8 *)CBB_BASE(MAP_CBB);
    for (int t = 0; t < 5; t++) {
        memcpy8(cbb0 + t * 32, bg_tiles[t], 32);
    }

    /* ── OBJ sprite tiles → OBJ tile memory ─────────────────────────────  */
    /* 5 sprite tiles, each 32 bytes                                        */
    for (int t = 0; t < 5; t++) {
        memcpy8(MEM_OBJ_TILES + t * 32, obj_tiles[t], 32);
    }
}
