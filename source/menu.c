/*
 * menu.c  –  Hearthwood Hollow title screen.
 *
 * Rendered in GBA Mode 3 (240×160 16-bit bitmap).
 * All colours are exact conversions of pixel-art.jsx PAL object → GBA_RGB.
 *
 * Scene (faithful to sceneTitle in scenes.jsx):
 *   • Dawn sky gradient: sky4 (indigo) → sky3 (purple) → sky2 (peach) →
 *                        sky1 (gold) → horizon peach
 *   • Dithered tier boundaries at rows 17, 31, 51
 *   • Warm golden sun top-right
 *   • Distant indigo mountains (sky4)
 *   • Mid purple hills (#4a4a78)
 *   • Rolling foreground hills (grassD / grassL / grass layered)
 *   • Log cabin: trunkL walls, red shingle roof, stoneD chimney, waterL windows
 *   • Chimney smoke wisps (stoneL)
 *   • Two bird silhouettes '<' in sky
 *   • Three flanking trees (ellipse canopy, four leaf tones)
 *   • Grass scatter tufts + three flowers
 *   • Gold-trimmed wooden title plate "HEARTHWOOD" / "HOLLOW" (2× font)
 *   • Subtitle "A COZY COLLECTING TALE" (1× font)
 *   • Blinking "PRESS START" box (uiBlue, goldL border) at y=132
 *   • Copyright line
 *
 * Controls: A or START → MENU_ACTION_NEW_GAME
 */

#include "menu.h"
#include "gba.h"
#include "input.h"

/* ── Mode 3 pixel access ─────────────────────────────────────────────────── */
#define M3_VRAM  ((volatile u16 *)0x06000000)
#define PIX(x,y) M3_VRAM[(y)*240+(x)]

/* ── Colours — pixel-art.jsx PAL → GBA_RGB ───────────────────────────────── */

/* Sky (dawn gradient, top → bottom) */
#define C_SKY4    GBA_RGB(11,11,19)  /* PAL.sky4  #5a5e98  zenith indigo   */
#define C_SKY3    GBA_RGB(21,15,21)  /* PAL.sky3  #a87aa8  mid purple      */
#define C_SKY2    GBA_RGB(29,21,17)  /* PAL.sky2  #e9a888  warm peach      */
#define C_SKY1    GBA_RGB(31,27,20)  /* PAL.sky1  #f9d9a0  golden sky      */
#define C_HORIZ   GBA_RGB(30,25,18)  /*           #f4c890  horizon peach   */
#define C_SKY5    GBA_RGB( 5, 5,11)  /* PAL.sky5  #2a2e58  birds / shadow  */

/* Sun */
#define C_SUN1    GBA_RGB(30,28,20)  /*           #f6e0a0  outer glow      */
#define C_SUN2    GBA_RGB(31,29,23)  /*           #fce9b8  inner ring      */
#define C_WHITE   GBA_RGB(31,30,28)  /* PAL.white #f8f4e0  core            */

/* Terrain */
#define C_GRASSD  GBA_RGB( 5, 9, 3)  /* PAL.grassD #2a4a1a dark grass      */
#define C_GRASS   GBA_RGB( 8,13, 4)  /* PAL.grass  #446a26 mid grass       */
#define C_GRASSL  GBA_RGB(13,17, 7)  /* PAL.grassL #688a3a light grass     */
#define C_GRASSH  GBA_RGB(19,23,10)  /* PAL.grassH #9cbf52 highlight tuft  */
#define C_MIDHILL GBA_RGB( 9, 9,15)  /*            #4a4a78 mid-range hills  */

/* Foliage */
#define C_LEAF1   GBA_RGB( 2, 5, 2)  /* PAL.leaf1 #152e10                  */
#define C_LEAF2   GBA_RGB( 4, 9, 3)  /* PAL.leaf2 #244a18                  */
#define C_LEAF3   GBA_RGB( 7,13, 4)  /* PAL.leaf3 #386a26                  */
#define C_LEAF4   GBA_RGB(11,17, 6)  /* PAL.leaf4 #5e8a32                  */

/* Wood / trunk */
#define C_TRUNKD  GBA_RGB( 5, 3, 1)  /* PAL.trunkD #2a1808                 */
#define C_TRUNK   GBA_RGB( 9, 5, 2)  /* PAL.trunk  #4a2c14                 */
#define C_TRUNKL  GBA_RGB(13, 8, 4)  /* PAL.trunkL #6e4222  cabin walls    */
#define C_WOODD   GBA_RGB( 5, 2, 0)  /* PAL.woodD  #2a1606                 */
#define C_WOOD    GBA_RGB( 9, 4, 1)  /* PAL.wood   #48270e                 */
#define C_WOODM   GBA_RGB(13, 7, 3)  /* PAL.woodM  #6a3e1c                 */
#define C_WOODL   GBA_RGB(17,10, 5)  /* PAL.woodL  #8a542a                 */

/* Stone (chimney) */
#define C_STONED  GBA_RGB( 7, 7, 9)  /* PAL.stoneD #3a3848                 */
#define C_STONE   GBA_RGB(13,13,15)  /* PAL.stone  #6a6878                 */
#define C_STONEL  GBA_RGB(19,19,21)  /* PAL.stoneL #9a98a8  smoke wisps    */

/* Cabin roof (red shingles) */
#define C_REDD    GBA_RGB(15, 3, 2)  /* PAL.redD  #7a1a10                  */
#define C_RED     GBA_RGB(25, 7, 4)  /* PAL.red   #c83820                  */
#define C_REDL    GBA_RGB(29,11, 7)  /* PAL.redL  #e85838                  */

/* Window glass */
#define C_WATERL  GBA_RGB(13,17,25)  /* PAL.waterL #6e8ec8                 */

/* Gold UI */
#define C_GOLDD   GBA_RGB(17,11, 2)  /* PAL.goldD  #8a5e10                 */
#define C_GOLD    GBA_RGB(25,20, 5)  /* PAL.gold   #caa028                 */
#define C_GOLDL   GBA_RGB(30,25, 8)  /* PAL.goldL  #f0c844                 */
#define C_GOLDH   GBA_RGB(31,29,16)  /* PAL.goldH  #ffe884                 */

/* UI */
#define C_CREAM   GBA_RGB(30,28,22)  /* PAL.cream  #f4e4b4                 */
#define C_PARCH   GBA_RGB(29,26,19)  /* PAL.parch  #ecd49e                 */
#define C_SHADOW  GBA_RGB( 3, 1, 1)  /* PAL.shadow #1a0e08                 */
#define C_UIBLUE  GBA_RGB( 3, 5,10)  /* PAL.uiBlue #1a2a52                 */

/* Flowers */
#define C_FLOW    GBA_RGB(30,28,16)  /* PAL.flow   #f4e480 yellow flower    */
#define C_FLOW2   GBA_RGB(29,15,11)  /* PAL.flow2  #e87858 orange flower    */

/* ── Drawing helpers ─────────────────────────────────────────────────────── */
static void fill(int x, int y, int w, int h, u16 c)
{
    for (int j = y; j < y+h && j < 160; j++)
        for (int i = x; i < x+w && i < 240; i++)
            if (i >= 0 && j >= 0) PIX(i,j) = c;
}
static void hline(int x, int y, int w, u16 c)
{
    if (y < 0 || y >= 160) return;
    for (int i = x; i < x+w && i < 240; i++) if (i >= 0) PIX(i,y) = c;
}
static void vline(int x, int y, int h, u16 c)
{
    if (x < 0 || x >= 240) return;
    for (int j = y; j < y+h && j < 160; j++) if (j >= 0) PIX(x,j) = c;
}
static void circle_f(int cx, int cy, int r, u16 c)
{
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx+dy*dy <= r*r) {
                int px = cx+dx, py = cy+dy;
                if (px >= 0 && px < 240 && py >= 0 && py < 160)
                    PIX(px,py) = c;
            }
}

/* ── 5×7 pixel font (5-bit row, bit4=leftmost) ───────────────────────────── */
#define FONT_W 5
#define FONT_H 7
static const struct { char c; u8 r[7]; } fnt[] = {
    {'A',{0x04,0x0A,0x11,0x1F,0x11,0x11,0x11}},
    {'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'D',{0x1E,0x09,0x09,0x09,0x09,0x09,0x1E}},
    {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G',{0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}},
    {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I',{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M',{0x11,0x1B,0x15,0x11,0x11,0x11,0x11}},
    {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W',{0x11,0x11,0x11,0x15,0x15,0x1B,0x11}},
    {'X',{0x11,0x0A,0x0A,0x04,0x0A,0x0A,0x11}},
    {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'9',{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
    {'*',{0x00,0x11,0x0A,0x1F,0x0A,0x11,0x00}},
    {'/',{0x01,0x01,0x02,0x04,0x08,0x10,0x10}},
    {'<',{0x01,0x02,0x04,0x04,0x02,0x01,0x00}},  /* bird silhouette */
};
#define FONT_COUNT ((int)(sizeof(fnt)/sizeof(fnt[0])))

static void draw_char(int x, int y, char ch, u16 fg, u16 bg)
{
    for (int i = 0; i < FONT_COUNT; i++) {
        if (fnt[i].c != ch) continue;
        for (int row = 0; row < FONT_H; row++) {
            u8 bits = fnt[i].r[row];
            for (int col = 0; col < FONT_W; col++) {
                int px = x+col, py = y+row;
                if (px >= 0 && px < 240 && py >= 0 && py < 160) {
                    u16 colour = (bits & (0x10 >> col)) ? fg : bg;
                    if (colour != 0xFFFF) PIX(px,py) = colour; /* 0xFFFF = transparent */
                }
            }
        }
        return;
    }
}

/* Draw a char with transparent background (bg=0xFFFF skips bg pixels) */
static void draw_char_t(int x, int y, char ch, u16 fg)
{
    draw_char(x, y, ch, fg, 0xFFFF);
}

static int str_w(const char *s)
{
    int n = 0; while (s[n]) n++;
    return n > 0 ? n*(FONT_W+1)-1 : 0;
}

static void draw_str(int x, int y, const char *s, u16 fg, u16 bg)
{
    for (; *s; s++, x += FONT_W+1)
        draw_char(x, y, *s, fg, bg);
}

static void draw_str_c(int cx, int y, const char *s, u16 fg, u16 bg)
{
    draw_str(cx - str_w(s)/2, y, s, fg, bg);
}

/* 2× scale text for title plate — each font pixel → 2×2 block */
static void draw_str_2x(int x, int y, const char *s, u16 fg, u16 shadow)
{
    int cx = x;
    for (; *s; s++, cx += 10) {
        for (int i = 0; i < FONT_COUNT; i++) {
            if (fnt[i].c != *s) continue;
            for (int row = 0; row < FONT_H; row++) {
                u8 bits = fnt[i].r[row];
                for (int col = 0; col < FONT_W; col++) {
                    if (!(bits & (0x10 >> col))) continue;
                    int bx = cx + col*2, by = y + row*2;
                    fill(bx+1, by+1, 2, 2, shadow);
                    fill(bx,   by,   2, 2, fg);
                }
            }
            break;
        }
    }
}

/* ── Deterministic scatter hash ──────────────────────────────────────────── */
static int sc_hash(int seed, int i)
{
    u32 s = (u32)seed * 2654435761u + (u32)i * 374761393u;
    s ^= (s >> 15); s *= 2246822519u; s ^= (s >> 13);
    return (int)(s & 0x7FFFFFFFu);
}

/* ── Sprite helpers ───────────────────────────────────────────────────────── */

/* Round-canopy oak, ~22×26 px with trunk. Matches pixel-art.jsx drawTree(). */
static void draw_tree(int x, int y)
{
    const int cx = x+11, cy = y+11;
    for (int dy = -10; dy <= 10; dy++) {
        for (int dx = -11; dx <= 11; dx++) {
            int dd_n = dx*dx*100/(11*11) + dy*dy*100/(10*10);
            if (dd_n > 105) continue;
            u16 c;
            if      (dd_n > 92) c = C_LEAF1;
            else if (dd_n > 70) c = C_LEAF2;
            else if (dd_n > 40) c = C_LEAF3;
            else                c = C_LEAF4;
            int px = cx+dx, py = cy+dy;
            if (px >= 0 && px < 240 && py >= 0 && py < 160) PIX(px,py) = c;
        }
    }
    /* Trunk 4 wide */
    fill(cx-2, cy+10, 4, 5, C_TRUNK);
    fill(cx-2, cy+10, 1, 5, C_TRUNKD);
    fill(cx+1, cy+10, 1, 5, C_TRUNKD);
    /* Ground shadow */
    hline(cx-5, cy+15, 11, C_SHADOW);
}

/* Cozy cabin. Matches pixel-art.jsx drawHouse(ctx, x, y):
   red shingled roof, trunkL log walls, stoneD chimney, waterL windows. */
static void draw_house(int x, int y)
{
    /* Roof shingles (9 rows) */
    for (int r = 0; r < 9; r++) {
        int rw = 40 - r*2;
        u16 rc = (r < 2) ? C_REDD : ((r & 1) ? C_RED : C_REDL);
        hline(x+r, y+r, rw, rc);
    }
    /* Chimney */
    fill(x+6, y-4, 4, 6, C_STONED);
    hline(x+6, y-4, 4, C_STONE);
    fill(x+7, y-6, 2, 2, C_STONEL);
    /* Log walls */
    fill(x+2, y+9, 36, 14, C_TRUNKL);
    for (int r = 11; r < 23; r += 3) hline(x+2, y+r, 36, C_TRUNK);
    /* Left window */
    fill(x+6,  y+12, 7, 6, C_WOODD);
    fill(x+7,  y+13, 5, 4, C_WATERL);
    hline(x+7, y+15, 5, C_WOODD);     /* horizontal crossbar */
    vline(x+9, y+13, 4, C_WOODD);     /* vertical crossbar   */
    /* Right window */
    fill(x+27, y+12, 7, 6, C_WOODD);
    fill(x+28, y+13, 5, 4, C_WATERL);
    hline(x+28, y+15, 5, C_WOODD);
    vline(x+30, y+13, 4, C_WOODD);
    /* Door */
    fill(x+17, y+14, 7, 9, C_WOODD);
    fill(x+18, y+15, 5, 8, C_WOOD);
    PIX(x+22, y+19) = C_GOLDL;        /* door knob */
    /* Foundation */
    fill(x,   y+23, 40, 2, C_STONED);
    hline(x+1, y+24, 38, C_STONE);
}

/* ── Title panel (184×56, gold frame, wood fill, jewel corners) ──────────── */
static void draw_title_panel(void)
{
    const int px0 = 28, py0 = 14, pw = 184, ph = 56;

    /* Drop shadow */
    fill(px0+3, py0+3, pw, ph, C_SHADOW);
    /* Gold outer frame */
    fill(px0,   py0,   pw,   ph,   C_GOLDD);
    fill(px0+1, py0+1, pw-2, ph-2, C_GOLD);
    hline(px0+1, py0+1, pw-2, C_GOLDL);
    vline(px0+1, py0+1, ph-2, C_GOLDL);
    /* Wood inner fill */
    fill(px0+3, py0+3, pw-6, ph-6, C_WOOD);
    /* Wood top/bottom edge accents */
    hline(px0+3, py0+3,    pw-6, C_WOODD);
    hline(px0+3, py0+ph-4, pw-6, C_WOODM);
    /* Wood grain lines */
    for (int i = 0; i < pw-6; i++) {
        if (i % 3 == 0) PIX(px0+3+i, py0+8)  = C_WOODM;
        if (i % 5 == 1) PIX(px0+3+i, py0+24) = C_WOODM;
        if (i % 4 == 2) PIX(px0+3+i, py0+44) = C_WOODD;
    }
    /* Jewel corner pegs (4×4 each, per pixel-art.jsx drawPanel) */
    const int jxs[4] = {px0+5, px0+pw-9, px0+5,    px0+pw-9};
    const int jys[4] = {py0+5, py0+5,    py0+ph-9, py0+ph-9};
    for (int c = 0; c < 4; c++) {
        fill(jxs[c], jys[c], 4, 4, C_REDD);
        hline(jxs[c]+1, jys[c]+1, 2, C_REDL);
        PIX(jxs[c],   jys[c])   = C_GOLD;
        PIX(jxs[c]+3, jys[c])   = C_GOLD;
        PIX(jxs[c],   jys[c]+3) = C_GOLD;
        PIX(jxs[c]+3, jys[c]+3) = C_GOLD;
    }
    /* Big 2× title */
    draw_str_2x(px0+14, py0+10, "HEARTHWOOD", C_GOLDL, C_WOODD);
    draw_str_2x(px0+40, py0+28, "HOLLOW",     C_GOLDL, C_WOODD);
    /* Subtitle (1×, centered) */
    draw_str_c(120, py0+47, "A COZY COLLECTING TALE", C_CREAM, C_WOOD);
}

/* ── PRESS START button (y=132, w=72, centred at x=118) ─────────────────── */
static void draw_press_start(bool blink)
{
    const int bx = 82, by = 132, bw = 72, bh = 14;
    if (blink) {
        /* Shadow */
        fill(bx+2, by+2, bw, bh, C_SHADOW);
        /* Blue box */
        fill(bx, by, bw, bh, C_UIBLUE);
        /* Gold border */
        hline(bx,      by,      bw, C_GOLDL);
        hline(bx,      by+bh-1, bw, C_GOLDL);
        vline(bx,      by,      bh, C_GOLDL);
        vline(bx+bw-1, by,      bh, C_GOLDL);
        /* Text centred at 118 */
        draw_str_c(118, by+4, "PRESS START", C_CREAM, C_UIBLUE);
    } else {
        /* Erase: repaint grass over the button area */
        fill(bx-2, by-2, bw+5, bh+5, C_GRASS);
        /* Sprinkle tufts so the erase blends with the hill */
        for (int i = 0; i < 24; i++) {
            int sx = bx-2 + sc_hash(77, i*2)   % (bw+5);
            int sy = by-2 + sc_hash(77, i*2+1) % (bh+5);
            if (sx >= 0 && sx < 240 && sy >= 0 && sy < 160)
                PIX(sx, sy) = (i & 1) ? C_GRASSL : C_GRASSH;
        }
    }
}

/* ── Full scene draw ─────────────────────────────────────────────────────── */
static void draw_full_title(bool show_prompt)
{
    /* ── Sky gradient (rows 0–99) ─────────────────────────────────────── */
    for (int y = 0; y < 100; y++) {
        u16 c;
        if      (y < 18) c = C_SKY4;
        else if (y < 32) c = C_SKY3;
        else if (y < 52) c = C_SKY2;
        else if (y < 75) c = C_SKY1;
        else             c = C_HORIZ;
        hline(0, y, 240, c);
    }
    /* Dither seams (odd pixels only, one tier lighter) */
    for (int x = 1; x < 240; x += 2) {
        PIX(x, 17) = C_SKY3;
        PIX(x, 31) = C_SKY2;
        PIX(x, 51) = C_SKY1;
    }

    /* ── Sun (top-right, warm golden) ────────────────────────────────── */
    circle_f(200, 60, 13, C_SUN1);
    circle_f(200, 60, 10, C_SUN2);
    circle_f(198, 58,  4, C_WHITE);

    /* ── Distant mountains (sky4 colour) ──────────────────────────────── */
    for (int x = 0; x < 240; x++) {
        int h = 18 + (((x*7)%80 < 40) ? (x*7)%40 : 80-(x*7)%80)/5
                   + (((x*21+130)%60 < 30) ? (x*21+130)%30 : 60-(x*21+130)%60)/6;
        for (int dy = 0; dy < h; dy++) {
            int py = 100 - h + dy;
            if (py >= 0 && py < 160) PIX(x, py) = C_SKY4;
        }
    }
    /* Mid-range purple hills (#4a4a78) */
    for (int x = 0; x < 240; x++) {
        int h = 10 + (((x*5+170)%60 < 30) ? (x*5+170)%30 : 60-(x*5+170)%60)/5
                   + (((x*18)%50 < 25) ? (x*18)%25 : 50-(x*18)%50)/8;
        for (int dy = 0; dy < h; dy++) {
            int py = 100 - h + dy;
            if (py >= 0 && py < 160) PIX(x, py) = C_MIDHILL;
        }
    }

    /* ── Foreground rolling hills ─────────────────────────────────────── */
    for (int x = 0; x < 240; x++) {
        int h = 25 + (((x*25)%80 < 40) ? (x*25)%40 : 80-(x*25)%80)/3
                   + (((x*9+20)%40 < 20) ? (x*9+20)%20 : 40-(x*9+20)%40)/4;
        int base = 100 - h + 28;
        /* Column: grassL at lip, grass next 2, grassD rest */
        for (int dy = 0; base+dy < 160 && dy < 80; dy++) {
            u16 gc = (dy < 3) ? C_GRASSL : (dy < 5) ? C_GRASS : C_GRASSD;
            if (base+dy >= 0) PIX(x, base+dy) = gc;
        }
    }

    /* ── Cabin (pos 30,96 matching drawHouse(ctx, 30, 96)) ───────────── */
    draw_house(30, 96);
    /* Chimney smoke wisps (three wisps, stoneL, slight wavy offset) */
    PIX(36, 78) = C_STONEL; PIX(37, 78) = C_STONEL;
    PIX(35, 74) = C_STONEL; PIX(36, 74) = C_STONEL;
    PIX(37, 70) = C_STONEL; PIX(38, 70) = C_STONEL;

    /* ── Bird silhouettes '<' in the sky ─────────────────────────────── */
    draw_char_t(90, 28, '<', C_SKY5);
    draw_char_t(98, 30, '<', C_SKY5);

    /* ── Trees flanking ──────────────────────────────────────────────── */
    draw_tree(180, 88);
    draw_tree(210, 100);
    draw_tree(  6, 110);

    /* ── Grass scatter tufts (grassH) ───────────────────────────────── */
    {
        int count = 240 * 40 * 5 / 100;  /* density 0.05 over 240×40 */
        for (int i = 0; i < count; i++) {
            int sx = sc_hash(9, i*2)   % 240;
            int sy = 120 + sc_hash(9, i*2+1) % 40;
            if (sx < 240 && sy < 160) PIX(sx, sy) = C_GRASSH;
        }
    }

    /* ── Flowers (exact positions from sceneTitle) ───────────────────── */
    PIX(70,  134) = C_FLOW;   PIX(71, 134) = C_FLOW;   /* yellow pair    */
    PIX(130, 142) = C_FLOW2;                             /* orange-red     */
    PIX(155, 130) = C_FLOW;                              /* yellow         */

    /* ── Title plate ─────────────────────────────────────────────────── */
    draw_title_panel();

    /* ── PRESS START ─────────────────────────────────────────────────── */
    draw_press_start(show_prompt);

    /* ── Copyright ───────────────────────────────────────────────────── */
    draw_str(4, 152, "* HEARTH STUDIO 1991", C_PARCH, C_GRASSD);
}

/* ── Module state ────────────────────────────────────────────────────────── */
static int  title_frame = 0;
static bool last_blink  = true;

/* ── Public API ──────────────────────────────────────────────────────────── */
void menu_init(void)
{
    title_frame = 0;
    last_blink  = true;

    /* Set Mode 3 + BG2 enable.  Hide all OAM sprites. */
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
    for (int i = 0; i < 128; i++) {
        OAM_MEM[i].attr0 = OBJ_HIDE;
        OAM_MEM[i].attr1 = 0;
        OAM_MEM[i].attr2 = 0;
    }

    draw_full_title(true);
}

MenuAction menu_update(void)
{
    title_frame++;

    /* Blink "PRESS START" every ~30 frames (~0.5 s at 60 Hz) */
    bool blink = ((title_frame / 30) & 1) == 0;
    if (blink != last_blink) {
        draw_press_start(blink);
        last_blink = blink;
    }

    if (input_pressed(KEY_A) || input_pressed(KEY_START))
        return MENU_ACTION_NEW_GAME;

    return MENU_ACTION_NONE;
}

int menu_get_selected_map(void)
{
    return 0;
}
