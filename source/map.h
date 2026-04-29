/*
 * map.h  –  Tile-map definitions and API.
 *
 * The playfield is MAP_W × MAP_H tiles (each tile 8×8 pixels).
 * The GBA BG hardware is configured for a 64×32-tile screen-size, which
 * neatly covers our 64×32 map and gives a scroll range of:
 *   horizontal: (64×8 − 240) = 272 pixels
 *   vertical:   (32×8 − 160) =  96 pixels
 */
#pragma once
#include "gba.h"

/* Map dimensions (in tiles) */
#define MAP_W    64
#define MAP_H    32

/* Terrain tile indices (match bg_tiles[] in tiles.h) */
#define TILE_GRASS    0
#define TILE_WATER    1
#define TILE_FOREST   2
#define TILE_MOUNTAIN 3
#define TILE_DIRT     4

/* Map pixel extents */
#define MAP_PX_W   (MAP_W * 8)   /* 512 */
#define MAP_PX_H   (MAP_H * 8)   /* 256 */

/* Maximum camera scroll positions (pixels) */
#define CAM_MAX_X  (MAP_PX_W - SCREEN_W)   /* 272 */
#define CAM_MAX_Y  (MAP_PX_H - SCREEN_H)   /*  96 */

/* Load the fixed map data into VRAM and reset the camera */
void map_init(void);

/*
 * Set the hardware BG scroll registers to (cam_x, cam_y).
 * Call once per frame after game logic has updated the camera.
 */
void map_set_scroll(s16 cam_x, s16 cam_y);

/* Returns the tile type at tile position (tx, ty) */
u8 map_get_tile(int tx, int ty);

/* Returns true when the tile at (tx, ty) blocks movement */
bool map_tile_blocks(int tx, int ty);
