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
/* CBB 0  → VRAM 0x06000000 (BG tile graphics, shared by BG0 and BG1)       */
/* SBB 24 → VRAM 0x0600C000 (BG0 tile map, first 2 KB of a 64×32 map)       */
/* SBB 25 → VRAM 0x0600C800 (BG0 tile map, second 2 KB – right half)        */
/* SBB 26 → VRAM 0x0600D000 (BG1 tile map – fixed HUD strip, 32×32)         */
#define MAP_CBB  0
#define MAP_SBB  24
#define HUD_SBB  26

#define HUD_TILE_CLEAR  6   /* all-transparent tile (BG palette index 0)    */
#define HUD_TILE_STRIP  7   /* warm-brown strip with gold bottom edge        */

static void vram_copy_tile(volatile u8 *dst, const u8 *src)
{
    volatile u16 *d = (volatile u16 *)dst;
    for (int i = 0; i < 16; i++) {
        u16 lo = src[i * 2 + 0];
        u16 hi = src[i * 2 + 1];
        d[i] = (u16)(lo | (hi << 8));
    }
}

void video_init(void)
{
    /* ── Display control: BG0 (world) + BG1 (HUD strip) + OBJ ─────────── */
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;

    /* ── BG0: scrolling world map – CBB 0, SBB 24, 4-bpp, 64×32 ────────  */
    REG_BG0CNT  = BG_CBB(MAP_CBB) | BG_SBB(MAP_SBB) | BG_4BPP
                | BG_SIZE_64x32 | BG_PRIO(3);

    /* ── BG palette ──────────────────────────────────────────────────────  */
    memcpy16(MEM_PAL_BG, bg_palette, 16);

    /* ── OBJ palette ─────────────────────────────────────────────────────  */
    memcpy16(MEM_PAL_OBJ, obj_palette, 16);

    /* ── BG tile graphics → CBB 0 ───────────────────────────────────────  */
    /* Each tile is 32 bytes; 5 tiles total                                 */
    volatile u8 *cbb0 = (volatile u8 *)CBB_BASE(MAP_CBB);
    int bg_tile_count = sizeof(bg_tiles) / 32;
    for (int t = 0; t < bg_tile_count; t++) {
        vram_copy_tile(cbb0 + t * 32, bg_tiles[t]);
    }

    /* ── OBJ sprite tiles → OBJ tile memory ─────────────────────────────  */
    int obj_tile_count = sizeof(obj_tiles) / 32;
    for (int t = 0; t < obj_tile_count; t++) {
        vram_copy_tile(MEM_OBJ_TILES + t * 32, obj_tiles[t]);
    }

    /* ── BG1: fixed HUD strip – CBB 0, SBB 26, 4-bpp, 32×32 ───────────  */
    REG_BG1CNT  = BG_CBB(MAP_CBB) | BG_SBB(HUD_SBB) | BG_4BPP
                | BG_SIZE_32x32 | BG_PRIO(0);
    REG_BG1HOFS = 0;
    REG_BG1VOFS = 0;

    /* Fill BG1 tile map: row 0 = HUD strip, rows 1-31 = transparent */
    volatile u16 *hud_map = SBB_BASE(HUD_SBB);
    for (int i = 0; i < 32 * 32; i++)
        hud_map[i] = (u16)HUD_TILE_CLEAR;
    for (int tx = 0; tx < 32; tx++)
        hud_map[tx] = (u16)HUD_TILE_STRIP;
}
