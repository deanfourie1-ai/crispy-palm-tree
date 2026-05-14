/*
 * map.c  –  Hearthwood Hollow world map.
 *
 * 64×32 tile terrain (each tile 8×8 px → 512×256 world pixels):
 *   G = Grass (0)   W = Water/Pond (1)   F = Forest (2)
 *   M = Mountain (3)  D = Dirt path (4)   O = Ore (5)
 *
 * Region layout:
 *   OAK GLADE      – forest cluster top-left   (rows 0-8,  cols 0-16)
 *   MARROW CAVE    – ore/mountain top-right     (rows 0-8,  cols 44-63)
 *   CROSSROADS     – dirt path centre           (rows 14-16, cols 0-63)
 *   WILLOW'S HOVEL – open grass centre          (rows 12-20, cols 20-32)
 *   PONDSHORE      – water cluster bottom-left  (rows 20-28, cols 2-14)
 *   DRIFTWOOD COVE – scattered forest bottom-rt (rows 20-28, cols 48-60)
 *   Mountain border surrounds the map edges.
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
#define O TILE_ORE

static MapId active_map = MAP_ID_1_PLACEHOLDER;

static const MapMetadata map_metadata[MAP_ID_COUNT] = {
    {
        .name        = "HEARTHWOOD HOLLOW",
        .description = "A COZY COLLECTING TALE",
        .preview_base   = GBA_RGB(8, 13, 5),    /* grass green */
        .preview_accent = GBA_RGB(20, 15, 5),   /* gold */
    },
    {
        .name        = "RESOURCE TEST",
        .description = "FAST MINI-GAME TEST",
        .preview_base   = GBA_RGB(8, 18, 8),
        .preview_accent = GBA_RGB(24, 18, 4),
    },
};

/* ── Map data  64 columns × 32 rows ─────────────────────────────────────── */
/*
 * OAK GLADE:     dense forest top-left  (cols  0-18, rows 0-8)
 * MARROW CAVE:   ore + mountain top-right (cols 44-63, rows 0-8)
 * PATH:          dirt road centre horizontally (row 15, col 0-63)
 * HOVEL AREA:    clear grass centre (cols 20-36, rows 10-20)
 * PONDSHORE:     water pool bottom-left (cols 2-12, rows 20-27)
 * DRIFTWOOD:     scattered forest bottom-right (cols 48-60, rows 20-28)
 */
static u8 map_data[MAP_H][MAP_W] = {
 /* row  0 */ {F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M},
 /* row  1 */ {F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,M,M,O,O,M,M,M,M,M,O,O,M,M,M,M,M,M,M,M,M,M},
 /* row  2 */ {F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,M,O,O,O,O,M,M,M,O,O,O,O,M,M,M,M,M,M,M,M,M},
 /* row  3 */ {F,F,F,G,G,G,F,F,F,F,F,F,F,G,G,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,M,O,O,O,O,O,O,M,O,O,O,O,O,O,M,M,M,M,M,M,M,M},
 /* row  4 */ {F,F,G,G,G,G,G,F,F,F,F,F,G,G,G,G,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,M,O,O,O,O,O,O,O,O,O,O,O,O,O,M,M,M,M,M,M,M,M},
 /* row  5 */ {F,G,G,G,G,G,G,G,F,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,M,M,O,O,M,M,O,O,M,M,O,O,M,M,O,O,M,M,M,M,M,M,M},
 /* row  6 */ {G,G,G,G,G,G,G,G,G,F,F,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M},
 /* row  7 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row  8 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row  9 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 10 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 11 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 12 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 13 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 14 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 15 */ {D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D},
 /* row 16 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 17 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 18 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 19 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 20 */ {G,G,W,W,W,W,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F,F,F,F,F,F,F,G,G,G,G,G,G,G,G},
 /* row 21 */ {G,G,W,W,W,W,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,F,G,G,G,G,F,F,F,G,G,G,G,G,G,G,G},
 /* row 22 */ {G,G,W,W,W,W,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,F,G,G,G,G,G,G,G,F,G,G,G,G,G,G,G,G},
 /* row 23 */ {G,G,W,W,W,W,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 24 */ {G,G,G,W,W,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 25 */ {G,G,G,G,W,W,W,W,W,W,W,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 26 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 27 */ {G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
 /* row 28 */ {M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M},
 /* row 29 */ {M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M},
 /* row 30 */ {M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M},
 /* row 31 */ {M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M},
};

static u8   map1_snapshot[MAP_H][MAP_W];
static bool map1_snapshot_ready = false;

static void map_snapshot_if_needed(void)
{
    if (map1_snapshot_ready) return;

    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            map1_snapshot[y][x] = map_data[y][x];

    map1_snapshot_ready = true;
}

static void map_restore_map1_snapshot(void)
{
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            map_data[y][x] = map1_snapshot[y][x];
}

static void map_build_resource_test(void)
{
    /* Base field + mountain border so camera edges remain clearly bounded. */
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            bool border = (x == 0 || x == MAP_W - 1 || y == 0 || y == MAP_H - 1);
            map_data[y][x] = border ? TILE_MOUNTAIN : TILE_GRASS;
        }
    }

    /* Small road lane through the player side for quick path checks. */
    for (int x = 2; x < 24; x++) {
        map_data[24][x] = TILE_DIRT;
    }

    /* Forest cluster near peasant start. */
    map_data[20][7] = TILE_FOREST;
    map_data[20][8] = TILE_FOREST;
    map_data[20][9] = TILE_FOREST;
    map_data[21][8] = TILE_FOREST;

    /* Water pool near peasant start (fish by standing adjacent). */
    for (int y = 20; y <= 22; y++)
        for (int x = 12; x <= 15; x++)
            map_data[y][x] = TILE_WATER;

    /* Ore pocket near peasant start (ore is blocking, gather from adjacent). */
    map_data[22][5] = TILE_ORE;
    map_data[22][6] = TILE_ORE;
    map_data[23][6] = TILE_ORE;
    map_data[23][5] = TILE_MOUNTAIN;

    /* Extra distant resources for quick long-path sanity checks. */
    map_data[6][50] = TILE_FOREST;
    map_data[7][52] = TILE_ORE;
    map_data[9][46] = TILE_WATER;
    map_data[9][47] = TILE_WATER;
}

#undef G
#undef W
#undef F
#undef M
#undef D
#undef O

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
    return (t == TILE_WATER) || (t == TILE_MOUNTAIN) || (t == TILE_ORE);
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
    map_snapshot_if_needed();

    switch (active_map) {
        case MAP_ID_1_PLACEHOLDER:
            map_restore_map1_snapshot();
            break;
        case MAP_ID_2_RESOURCE_TEST:
            map_build_resource_test();
            break;
        default:
            map_restore_map1_snapshot();
            break;
    }

    volatile u16 *sbb_l = SBB_BASE(MAP_SBB_L);
    volatile u16 *sbb_r = SBB_BASE(MAP_SBB_R);

    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < 32; tx++) {
            sbb_l[ty * 32 + tx] = map_data[ty][tx];
            sbb_r[ty * 32 + tx] = map_data[ty][tx + 32];
        }
    }
}

void map_set_active(MapId id)
{
    if (id >= MAP_ID_COUNT) return;
    active_map = id;
}

MapId map_get_active(void)
{
    return active_map;
}

int map_get_count(void)
{
    return MAP_ID_COUNT;
}

const MapMetadata *map_get_metadata(MapId id)
{
    if (id >= MAP_ID_COUNT) return 0;
    return &map_metadata[id];
}

/* ── Single-tile update (runtime depletion) ─────────────────────────────── */
void map_set_tile(int tx, int ty, u8 tile)
{
    if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) return;
    map_data[ty][tx] = tile;
    volatile u16 *sbb = (tx < 32) ? SBB_BASE(MAP_SBB_L) : SBB_BASE(MAP_SBB_R);
    int lx = (tx < 32) ? tx : tx - 32;
    sbb[ty * 32 + lx] = tile;
}

/* ── Scroll register update ──────────────────────────────────────────────── */
void map_set_scroll(s16 cam_x, s16 cam_y)
{
    REG_BG0HOFS = (u16)cam_x;
    REG_BG0VOFS = (u16)cam_y;
}
