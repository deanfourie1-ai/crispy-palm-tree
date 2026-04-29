/*
 * tiles.h  –  Inline graphics data for background tiles and OBJ sprites.
 *
 * Background tiles (4-bpp, 8×8 pixels each = 32 bytes):
 *   0 – Grass        palette colours 1 (light green) / 2 (mid green)
 *   1 – Water        palette colours 4 (light blue)  / 5 (dark blue)
 *   2 – Forest       palette colours 3 (dark green)  / 2 (mid green)
 *   3 – Mountain     palette colours 6 (light grey)  / 7 (dark grey)
 *   4 – Dirt / road  palette colours 8 (brown)       / 9 (tan)
 *
 * In 4-bpp format each byte packs 2 pixels:
 *   bits 3:0 = left / even-x pixel
 *   bits 7:4 = right / odd-x pixel
 *
 * OBJ sprite tiles (4-bpp, 8×8 = 32 bytes, in OBJ tile memory):
 *   0 – Footman  (player unit 1)  OBJ palette colours 2 (blue) / 1 (skin) / 3
 *   1 – Orc      (enemy unit)     OBJ palette colours 4 (orc green) / 5 / 6
 *   2 – Cursor   (selection box)  OBJ palette colour  8 (yellow)
 *   3 – Peasant  (player unit 2)  OBJ palette colours 12 (brown) / 1 (skin)
 *   4 – Highlight (selected unit) OBJ palette colour  8 (yellow)
 */

#pragma once
#include "gba.h"

/* ── BG tile graphics data ───────────────────────────────────────────────── */

static const u8 bg_tiles[5][32] = {
    /* 0 – Grass: mostly colour 1, occasional colour 2 speckles */
    {
        0x11, 0x21, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x21,
        0x21, 0x11, 0x11, 0x11,
        0x11, 0x12, 0x11, 0x11,
        0x11, 0x11, 0x21, 0x11,
        0x12, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x12,
        0x11, 0x11, 0x11, 0x11,
    },
    /* 1 – Water: colour 4 base, colour 5 ripple speckles */
    {
        0x44, 0x54, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x54, 0x44,
        0x54, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x54,
        0x44, 0x44, 0x44, 0x44,
        0x44, 0x54, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44,
    },
    /* 2 – Forest: colour 3 base, colour 2 highlights */
    {
        0x33, 0x23, 0x33, 0x33,
        0x33, 0x33, 0x23, 0x33,
        0x23, 0x33, 0x33, 0x33,
        0x33, 0x33, 0x33, 0x23,
        0x33, 0x23, 0x33, 0x33,
        0x33, 0x33, 0x33, 0x33,
        0x23, 0x33, 0x23, 0x33,
        0x33, 0x33, 0x33, 0x33,
    },
    /* 3 – Mountain: colour 6 base, colour 7 shadowed cracks */
    {
        0x66, 0x66, 0x67, 0x66,
        0x66, 0x76, 0x66, 0x66,
        0x67, 0x66, 0x66, 0x67,
        0x76, 0x66, 0x77, 0x66,
        0x66, 0x77, 0x66, 0x66,
        0x67, 0x66, 0x66, 0x77,
        0x76, 0x66, 0x67, 0x66,
        0x66, 0x76, 0x66, 0x66,
    },
    /* 4 – Dirt / road: colour 8 base, colour 9 pebble speckles */
    {
        0x88, 0x98, 0x88, 0x88,
        0x88, 0x88, 0x89, 0x88,
        0x98, 0x88, 0x88, 0x88,
        0x88, 0x88, 0x98, 0x88,
        0x88, 0x98, 0x88, 0x88,
        0x89, 0x88, 0x88, 0x88,
        0x88, 0x88, 0x88, 0x98,
        0x88, 0x88, 0x98, 0x88,
    },
};

/* ── OBJ sprite tile data ─────────────────────────────────────────────────  */

static const u8 obj_tiles[5][32] = {
    /* 0 – Footman: blue (2) armour, skin (1) face, dark-blue (3) outline */
    {
        0x20, 0x22, 0x22, 0x02,   /* row 0: .22222. */
        0x22, 0x11, 0x11, 0x22,   /* row 1: 22.hh.22 (head/face) */
        0x12, 0x11, 0x11, 0x21,   /* row 2: 2hhhhh2 */
        0x20, 0x22, 0x22, 0x02,   /* row 3: .22222. */
        0x22, 0x22, 0x22, 0x22,   /* row 4: 22222222 (torso) */
        0x22, 0x22, 0x22, 0x22,   /* row 5: 22222222 */
        0x02, 0x22, 0x22, 0x20,   /* row 6: .2.222.2 (legs) */
        0x00, 0x22, 0x22, 0x00,   /* row 7: ..2222.. (feet) */
    },
    /* 1 – Orc: orc-green (4) body, dark-green (5) outline, red (6) eyes */
    {
        0x45, 0x44, 0x44, 0x54,   /* row 0: 4444 (body outline) */
        0x44, 0x46, 0x64, 0x44,   /* row 1: ..66.. (red eyes) */
        0x44, 0x44, 0x44, 0x44,   /* row 2: */
        0x45, 0x44, 0x44, 0x54,   /* row 3: (highlight line) */
        0x54, 0x44, 0x44, 0x45,   /* row 4: torso */
        0x54, 0x44, 0x44, 0x45,   /* row 5: */
        0x54, 0x44, 0x44, 0x45,   /* row 6: legs */
        0x55, 0x55, 0x55, 0x55,   /* row 7: feet */
    },
    /* 2 – Cursor: hollow yellow (8) box, transparent interior */
    {
        0x88, 0x88, 0x88, 0x88,   /* row 0: top bar */
        0x08, 0x00, 0x00, 0x80,   /* row 1: |      | */
        0x08, 0x00, 0x00, 0x80,   /* row 2: |      | */
        0x08, 0x00, 0x00, 0x80,   /* row 3: |      | */
        0x08, 0x00, 0x00, 0x80,   /* row 4: |      | */
        0x08, 0x00, 0x00, 0x80,   /* row 5: |      | */
        0x08, 0x00, 0x00, 0x80,   /* row 6: |      | */
        0x88, 0x88, 0x88, 0x88,   /* row 7: bottom bar */
    },
    /* 3 – Peasant: brown (12=0xC) tunic, skin (1) face */
    {
        0xC0, 0xCC, 0xCC, 0x0C,   /* row 0: .CCCCCC. */
        0xCC, 0x11, 0x11, 0xCC,   /* row 1: CC.hh.CC (head) */
        0x1C, 0x11, 0x11, 0xC1,   /* row 2: Chhhhhc */
        0xC0, 0xCC, 0xCC, 0x0C,   /* row 3: .CCCCCC. */
        0xCC, 0xCC, 0xCC, 0xCC,   /* row 4: torso */
        0xCC, 0xCC, 0xCC, 0xCC,   /* row 5: */
        0x0C, 0xCC, 0xCC, 0xC0,   /* row 6: legs */
        0x00, 0xCC, 0xCC, 0x00,   /* row 7: feet */
    },
    /* 4 – Selection highlight: solid yellow (8) border, transparent fill */
    {
        0x88, 0x88, 0x88, 0x88,   /* row 0 */
        0x88, 0x88, 0x88, 0x88,   /* row 1 */
        0x88, 0x00, 0x00, 0x88,   /* row 2 */
        0x88, 0x00, 0x00, 0x88,   /* row 3 */
        0x88, 0x00, 0x00, 0x88,   /* row 4 */
        0x88, 0x00, 0x00, 0x88,   /* row 5 */
        0x88, 0x88, 0x88, 0x88,   /* row 6 */
        0x88, 0x88, 0x88, 0x88,   /* row 7 */
    },
};

/* ── Palette definitions ─────────────────────────────────────────────────── */

/*
 * BG palette 0 (16 colours):
 *   0 transparent  1 light-green  2 mid-green    3 dark-green
 *   4 light-blue   5 dark-blue    6 light-grey   7 dark-grey
 *   8 brown        9 tan         10 white        11 black
 *  12 yellow       13..15 unused (black)
 */
static const u16 bg_palette[16] = {
    GBA_RGB( 0,  0,  0),   /*  0 transparent / black       */
    GBA_RGB(15, 25,  8),   /*  1 light green   #7CCC44     */
    GBA_RGB( 8, 21,  8),   /*  2 mid green     #44AA44     */
    GBA_RGB( 4, 12,  0),   /*  3 dark green    #226600     */
    GBA_RGB( 8, 19, 31),   /*  4 light blue    #4499FF     */
    GBA_RGB( 0,  8, 21),   /*  5 dark blue     #0044AA     */
    GBA_RGB(21, 21, 21),   /*  6 light grey    #AAAAAA     */
    GBA_RGB(10, 10, 10),   /*  7 dark grey     #555555     */
    GBA_RGB(21, 12,  6),   /*  8 brown         #AA6633     */
    GBA_RGB(25, 21, 17),   /*  9 tan           #CCAA88     */
    GBA_RGB(31, 31, 31),   /* 10 white         #FFFFFF     */
    GBA_RGB( 0,  0,  0),   /* 11 black         #000000     */
    GBA_RGB(31, 31,  0),   /* 12 yellow        #FFFF00     */
    GBA_RGB( 0,  0,  0),   /* 13 unused                    */
    GBA_RGB( 0,  0,  0),   /* 14 unused                    */
    GBA_RGB( 0,  0,  0),   /* 15 unused                    */
};

/*
 * OBJ palette 0 (16 colours):
 *   0 transparent  1 skin          2 blue-armour   3 dark-blue
 *   4 orc-green    5 orc-dark      6 red           7 dark-red
 *   8 yellow       9 dark-yellow  10 white        11 black
 *  12 brown       13 tan          14..15 unused
 */
static const u16 obj_palette[16] = {
    GBA_RGB( 0,  0,  0),   /*  0 transparent               */
    GBA_RGB(31, 25, 19),   /*  1 skin          #FFCC99     */
    GBA_RGB( 8, 12, 25),   /*  2 blue armour   #4466CC     */
    GBA_RGB( 4,  6, 17),   /*  3 dark blue     #223388     */
    GBA_RGB( 8, 21,  8),   /*  4 orc green     #44AA44     */
    GBA_RGB( 4, 12,  0),   /*  5 orc dark      #226600     */
    GBA_RGB(25,  4,  4),   /*  6 red           #CC2222     */
    GBA_RGB(17,  0,  0),   /*  7 dark red      #880000     */
    GBA_RGB(31, 31,  0),   /*  8 yellow        #FFFF00     */
    GBA_RGB(17, 17,  0),   /*  9 dark yellow   #888800     */
    GBA_RGB(31, 31, 31),   /* 10 white         #FFFFFF     */
    GBA_RGB( 0,  0,  0),   /* 11 black         #000000     */
    GBA_RGB(21, 12,  6),   /* 12 brown         #AA6633     */
    GBA_RGB(25, 19, 10),   /* 13 tan           #CC9955     */
    GBA_RGB( 0,  0,  0),   /* 14 unused                    */
    GBA_RGB( 0,  0,  0),   /* 15 unused                    */
};
