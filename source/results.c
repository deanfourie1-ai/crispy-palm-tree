/*
 * results.c  –  Day-end results screen for Hearthwood Hollow.
 *
 * Rendered in GBA Mode 3 (240×160 16-bit bitmap).
 *
 * Layout:
 *   • Dusk-sky gradient background with distant hills + moon
 *   • Centred wooden panel "DAY X ENDS" header plaque
 *   • "TODAY'S HARVEST" subtitle
 *   • Three columns: WOOD  /  GOLD  /  FISH  with count + delta tag
 *   • Cozy streak meter (1–7 day pips)
 *   • Blinking "PRESS A TO REST" prompt at bottom
 *   • Auto-advance after RESULT_TIMEOUT frames (fallback safety net)
 */

#include "results.h"
#include "gba.h"
#include "game.h"   /* wood, gold, food, hovel_level */

/* ── Mode 3 helpers ────────────────────────────────────────────────────────── */
#define M3_VRAM      ((volatile u16 *)0x06000000)
#define R_PIX(x, y) M3_VRAM[(y) * 240 + (x)]

static void r_fill(int x, int y, int w, int h, u16 col)
{
    for (int j = y; j < y + h && j < 160; j++)
        for (int i = x; i < x + w && i < 240; i++)
            R_PIX(i, j) = col;
}
static void r_hline(int x, int y, int w, u16 col)
{
    for (int i = x; i < x + w && i < 240; i++) if (y >= 0 && y < 160) R_PIX(i, y) = col;
}
static void r_vline(int x, int y, int h, u16 col)
{
    for (int j = y; j < y + h && j < 160; j++) if (x >= 0 && x < 240) R_PIX(x, j) = col;
}

/* ── 5×7 pixel font (same subset used across Mode 3 screens) ──────────────── */
#define R_FONT_W 5
#define R_FONT_H 7
static const struct { char c; u8 r[7]; } rfont[] = {
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
    {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3',{0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}},
    {'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5',{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
    {'6',{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},
    {'7',{0x1F,0x01,0x02,0x04,0x04,0x04,0x04}},
    {'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9',{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
    {'+',{0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},
    {'/',{0x01,0x01,0x02,0x04,0x08,0x10,0x10}},
    {'*',{0x00,0x11,0x0A,0x1F,0x0A,0x11,0x00}},
    {'!',{0x04,0x04,0x04,0x04,0x04,0x00,0x04}},
};
#define RFONT_COUNT ((int)(sizeof(rfont)/sizeof(rfont[0])))

static void r_char(int x, int y, char ch, u16 fg, u16 bg)
{
    for (int i = 0; i < RFONT_COUNT; i++) {
        if (rfont[i].c != ch) continue;
        for (int row = 0; row < R_FONT_H; row++) {
            u8 bits = rfont[i].r[row];
            for (int col = 0; col < R_FONT_W; col++) {
                int px = x + col, py = y + row;
                if (px >= 0 && px < 240 && py >= 0 && py < 160)
                    R_PIX(px, py) = (bits & (0x10 >> col)) ? fg : bg;
            }
        }
        return;
    }
}

static int r_strw(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    return n > 0 ? n * (R_FONT_W + 1) - 1 : 0;
}

static void r_str(int x, int y, const char *s, u16 fg, u16 bg)
{
    for (; *s; s++, x += R_FONT_W + 1)
        r_char(x, y, *s, fg, bg);
}

static void r_str_c(int cx, int y, const char *s, u16 fg, u16 bg)
{
    r_str(cx - r_strw(s) / 2, y, s, fg, bg);
}

/* Tiny itoa: writes decimal number into buf (null-terminated), returns ptr */
static const char *r_itoa(int n, char *buf, int buflen)
{
    if (buflen < 2) { buf[0] = '\0'; return buf; }
    if (n < 0) n = 0;
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return buf; }
    int i = 0;
    while (n > 0 && i < buflen - 1) { buf[i++] = (char)('0' + n % 10); n /= 10; }
    buf[i] = '\0';
    /* reverse */
    for (int a = 0, b = i - 1; a < b; a++, b--) {
        char t = buf[a]; buf[a] = buf[b]; buf[b] = t;
    }
    return buf;
}

/* ── Colours ──────────────────────────────────────────────────────────────── */
/* Dusk sky */
#define RC_SKY0     GBA_RGB(10, 10, 24)   /* near-zenith deep indigo */
#define RC_SKY1     GBA_RGB(15, 12, 22)
#define RC_SKY2     GBA_RGB(20, 12, 20)
#define RC_SKY3     GBA_RGB(22, 14, 18)
#define RC_GRASSD   GBA_RGB( 5,  9,  3)   /* distant dark hills */
/* Wooden panel */
#define RC_WOOD     GBA_RGB( 9,  5,  1)
#define RC_WOOD_L   GBA_RGB(14,  8,  3)
#define RC_WOOD_D   GBA_RGB( 5,  3,  0)
/* Gold trim */
#define RC_GOLD_D   GBA_RGB(17, 12,  2)
#define RC_GOLD     GBA_RGB(25, 20,  5)
#define RC_GOLD_L   GBA_RGB(30, 25, 10)
#define RC_GOLD_H   GBA_RGB(31, 30, 18)
/* Text */
#define RC_CREAM    GBA_RGB(30, 28, 22)
#define RC_PARCH    GBA_RGB(29, 26, 20)
#define RC_SHADOW   GBA_RGB( 3,  2,  0)
/* UI blue */
#define RC_UIBLUE   GBA_RGB( 3,  5, 10)
#define RC_UIBLUEH  GBA_RGB(11, 15, 22)
/* Resource colours */
#define RC_GREEN    GBA_RGB( 7, 20,  5)   /* harvest positive delta */
#define RC_RED      GBA_RGB(25,  7,  4)

/* ── Module state ─────────────────────────────────────────────────────────── */
#define RESULT_TIMEOUT 600   /* auto-advance after ~10 s */

static int rs_day;
static int rs_wood;
static int rs_gold;
static int rs_fish;
static int rs_streak;
static int rs_timer;

/* ── Draw ─────────────────────────────────────────────────────────────────── */

static void draw_panel(int x, int y, int w, int h)
{
    /* drop shadow */
    r_fill(x+2, y+2, w, h, RC_SHADOW);
    /* gold border (outer → inner) */
    r_fill(x, y, w, h, RC_GOLD_D);
    r_fill(x+1, y+1, w-2, h-2, RC_GOLD);
    r_hline(x+1, y+1, w-2, RC_GOLD_L);
    r_vline(x+1, y+1, h-2, RC_GOLD_L);
    /* wood fill */
    r_fill(x+3, y+3, w-6, h-6, RC_WOOD);
    /* inner bevel */
    r_hline(x+3, y+3, w-6, RC_WOOD_D);
    r_vline(x+3, y+3, h-6, RC_WOOD_D);
    r_hline(x+3, y+h-4, w-6, RC_WOOD_L);
    r_vline(x+w-4, y+3, h-6, RC_WOOD_L);
    /* jewel corners */
    const int jxs[4] = {x+1, x+w-4, x+1, x+w-4};
    const int jys[4] = {y+1, y+1, y+h-4, y+h-4};
    for (int c = 0; c < 4; c++) {
        r_fill(jxs[c], jys[c], 3, 3, RC_RED);
        R_PIX(jxs[c]+1, jys[c]) = RC_GOLD_H;
        R_PIX(jxs[c], jys[c])   = RC_GOLD;
        R_PIX(jxs[c]+2, jys[c]) = RC_GOLD;
        R_PIX(jxs[c], jys[c]+2) = RC_GOLD;
        R_PIX(jxs[c]+2,jys[c]+2)= RC_GOLD;
    }
}

static void draw_plaque(int cx, int y, const char *s, int pw)
{
    int px = cx - pw/2;
    r_fill(px+1, y+1, pw, 13, RC_SHADOW);
    r_fill(px, y, pw, 13, RC_GOLD_D);
    r_fill(px+1, y+1, pw-2, 11, RC_GOLD);
    r_hline(px+1, y+1, pw-2, RC_GOLD_L);
    r_str_c(cx, y+4, s, RC_WOOD_D, RC_GOLD);
}

static void draw_resource_col(int cx, int top_y, const char *label,
                              int count, const char *icon_label, u16 icon_col)
{
    /* Big icon block */
    r_fill(cx-16, top_y, 32, 14, RC_UIBLUE);
    r_fill(cx-15, top_y+1, 30, 12, icon_col);
    r_str_c(cx, top_y+4, icon_label, RC_CREAM, icon_col);

    /* Delta tag above icon */
    char delta_buf[8];
    delta_buf[0] = '+';
    r_itoa(count, delta_buf+1, 6);
    int tagw = r_strw(delta_buf) + 6;
    r_fill(cx - tagw/2 - 1, top_y - 9, tagw + 2, 9, RC_SHADOW);
    r_fill(cx - tagw/2, top_y - 10, tagw, 9, RC_GREEN);
    r_hline(cx - tagw/2, top_y - 10, tagw, GBA_RGB(12, 28, 8));
    r_str_c(cx, top_y - 8, delta_buf, RC_CREAM, RC_GREEN);

    /* Label and count below icon */
    r_str_c(cx, top_y + 18, label, RC_PARCH, RC_UIBLUE);
    char cnt_buf[8];
    r_itoa(count, cnt_buf, 8);
    /* prefix "x" */
    char xbuf[10];
    xbuf[0] = 'X'; xbuf[1] = '\0';
    int bi = 0;
    while (cnt_buf[bi]) { xbuf[1 + bi] = cnt_buf[bi]; bi++; }
    xbuf[1 + bi] = '\0';
    r_str_c(cx, top_y + 27, xbuf, RC_GOLD_L, RC_UIBLUE);
}

static void draw_full_results(bool blink_prompt)
{
    /* ── Sky gradient ────────────────────────────────────────────────────── */
    for (int y = 0; y < 160; y++) {
        u16 c;
        if      (y < 30)  c = RC_SKY0;
        else if (y < 60)  c = RC_SKY1;
        else if (y < 90)  c = RC_SKY2;
        else if (y < 110) c = RC_SKY3;
        else              c = RC_GRASSD;
        r_hline(0, y, 240, c);
    }

    /* Stars */
    const int stars[][2] = {
        {14,5},{47,8},{91,3},{130,11},{167,4},{203,9},{227,6},
        {38,18},{72,14},{155,19},{188,15},{213,22},
        {8,25},{60,28},{105,23},{142,26},{198,28},{231,24},
    };
    for (int i = 0; i < 18; i++) {
        int sx = stars[i][0], sy = stars[i][1];
        if (sx < 240 && sy < 160) {
            R_PIX(sx, sy)   = (i%3 == 0) ? RC_CREAM : RC_PARCH;
            if (i % 5 == 0) R_PIX(sx+1, sy) = RC_GOLD_L;
        }
    }

    /* Moon (crescent) */
    {
        /* full circle */
        int mx = 38, my = 34, mr = 10;
        for (int dy = -mr; dy <= mr; dy++)
            for (int dx = -mr; dx <= mr; dx++)
                if (dx*dx + dy*dy <= mr*mr)
                    R_PIX(mx+dx, my+dy) = RC_CREAM;
        /* shadow bite */
        int bx = mx+4, by_off = 0;
        int br = 8;
        for (int dy = -br; dy <= br; dy++)
            for (int dx = -br; dx <= br; dx++)
                if (dx*dx + dy*dy <= br*br) {
                    int px = bx+dx, py = my+by_off+dy;
                    if (px >= 0 && px < 240 && py >= 0 && py < 160)
                        R_PIX(px, py) = RC_SKY0;
                }
    }

    /* Distant hills silhouette */
    for (int x = 0; x < 240; x++) {
        /* Two overlapping hill waves */
        int h1 = 14 + (int)(7 * (
            (x % 80 < 40) ? (x % 40) : (80 - x % 80)) / 40);
        int h2 = 8 + (int)(5 * (
            ((x+30) % 60 < 30) ? ((x+30) % 30) : (60 - (x+30) % 60)) / 30);
        int h = (h1 > h2) ? h1 : h2;
        for (int dy = 0; dy < h && (110 - h + dy) < 160; dy++)
            R_PIX(x, 110 - h + dy) = RC_GRASSD;
    }

    /* Cabin silhouette bottom-right */
    {
        int cx = 174, cy = 96;
        r_fill(cx, cy, 36, 18, GBA_RGB(3, 2, 1));
        r_fill(cx+6, cy+5, 6, 6, GBA_RGB(25, 20, 6));   /* window glow */
        r_fill(cx+22, cy+5, 6, 6, GBA_RGB(25, 20, 6));
        /* smoke */
        R_PIX(cx+7,  cy-2) = RC_PARCH;
        R_PIX(cx+8,  cy-5) = RC_PARCH;
        R_PIX(cx+7,  cy-8) = GBA_RGB(20,18,15);
    }

    /* ── Centre panel ────────────────────────────────────────────────────── */
    draw_panel(24, 18, 192, 112);

    /* Header plaque */
    {
        char hdr[24];
        /* build "* DAY X ENDS *" */
        const char *part1 = "* DAY ";
        const char *part2 = " ENDS *";
        char day_buf[6];
        r_itoa(rs_day, day_buf, 6);
        int pi = 0;
        for (int i = 0; part1[i]; i++) hdr[pi++] = part1[i];
        for (int i = 0; day_buf[i]; i++) hdr[pi++] = day_buf[i];
        for (int i = 0; part2[i]; i++) hdr[pi++] = part2[i];
        hdr[pi] = '\0';
        draw_plaque(120, 18, hdr, 90);
    }

    /* Subtitle */
    r_str_c(120, 38, "TODAY'S HARVEST", RC_CREAM, RC_WOOD);

    /* Three resource columns inside the panel (y ~ 52–88) */
    r_fill(26, 47, 188, 52, RC_UIBLUE);
    r_hline(26, 47, 188, RC_GOLD_D);
    r_hline(26, 98, 188, RC_GOLD_D);

    draw_resource_col(60,  60, "OAK LOGS",    rs_wood, "LOG", GBA_RGB(10,6,2));
    draw_resource_col(120, 60, "GOLD ORE",    rs_gold, "ORE", GBA_RGB(22,17,3));
    draw_resource_col(180, 60, "RIVER FISH",  rs_fish, "FISH",GBA_RGB(5,10,20));

    /* Column dividers */
    r_vline(90,  48, 50, RC_GOLD_D);
    r_vline(150, 48, 50, RC_GOLD_D);

    /* ── Cozy streak meter ───────────────────────────────────────────────── */
    r_fill(26, 102, 188, 22, RC_UIBLUE);
    r_fill(27, 103, 186, 20, RC_UIBLUE);
    r_str(30, 106, "COZY STREAK", RC_GOLD_L, RC_UIBLUE);
    {
        char sbuf[4];
        r_itoa(rs_streak, sbuf, 4);
        char s7[8]; s7[0]='0'+rs_streak; s7[1]='/'; s7[2]='7'; s7[3]='\0';
        if (rs_streak >= 10) { s7[0]='1'; s7[1]='0'+rs_streak-10; s7[2]='/'; s7[3]='7'; s7[4]='\0'; }
        r_str(200, 106, s7, RC_CREAM, RC_UIBLUE);
    }
    /* Pip bar */
    for (int i = 0; i < 7; i++) {
        int px = 30 + i * 22;
        bool filled = (i < rs_streak);
        u16 pip_col = filled ? RC_GOLD_L : RC_UIBLUEH;
        r_fill(px, 115, 18, 5, pip_col);
        if (filled) r_hline(px, 115, 18, RC_GOLD_H);
    }

    /* ── Bottom hints ────────────────────────────────────────────────────── */
    /* Upgrade row at y=131 is drawn separately by draw_upgrade_row()       */
    /* Press prompts */
    u16 pc = blink_prompt ? RC_CREAM : RC_PARCH;
    r_str_c(120, 148, "A:REST  SELECT:MAP", pc, RC_GRASSD);
}

/* ── World map (Step 6) ──────────────────────────────────────────────────── */

/* Hovel upgrade cost table indexed by current level → cost to next */
static const int hovel_wood_cost[HOVEL_MAX_LEVEL]  = { 20, 30, 45, 60 };
static const int hovel_gold_cost[HOVEL_MAX_LEVEL]  = {  0, 15, 25, 40 };
static const int hovel_fish_cost[HOVEL_MAX_LEVEL]  = {  0,  0,  0, 20 };

static const char *hovel_stage_name(int lvl) {
    switch (lvl) {
        case 0: return "TENT";
        case 1: return "CABIN";
        case 2: return "COTTAGE";
        case 3: return "LONGHOUSE";
        default: return "MANOR";
    }
}

static bool hovel_can_upgrade(void) {
    if (hovel_level >= HOVEL_MAX_LEVEL) return false;
    return (wood >= hovel_wood_cost[hovel_level] &&
            gold >= hovel_gold_cost[hovel_level] &&
            food >= hovel_fish_cost[hovel_level]);
}

/* Draw a labelled region box on the world map */
static void wmap_region(int x, int y, int w, int h, u16 col, const char *name)
{
    r_fill(x+1, y+1, w, h, RC_SHADOW);
    r_fill(x, y, w, h, col);
    r_fill(x+1, y+1, w-2, h-2, GBA_RGB(
        (int)((col & 0x1F) * 3 / 4),
        (int)(((col >> 5) & 0x1F) * 3 / 4),
        (int)(((col >> 10) & 0x1F) * 3 / 4)));
    r_hline(x+1, y+1, w-2, GBA_RGB(
        (int)((col & 0x1F) + 4 > 31 ? 31 : (col & 0x1F) + 4),
        (int)((((col>>5)&0x1F)+4>31)?31:((col>>5)&0x1F)+4),
        (int)((((col>>10)&0x1F)+4>31)?31:((col>>10)&0x1F)+4)));
    int lx = x + w/2 - (int)(r_strw(name)/2);
    int ly = y + h/2 - 3;
    if (ly < 0) ly = 0;
    r_str(lx+1, ly+1, name, RC_SHADOW, RC_SHADOW);
    r_str(lx,   ly,   name, RC_CREAM,
          GBA_RGB((col&0x1F)*3/4, ((col>>5)&0x1F)*3/4, ((col>>10)&0x1F)*3/4));
}

static void draw_world_map(void)
{
    /* Parchment background */
    for (int y = 0; y < 160; y++) {
        int t = y * 4 / 160;
        u16 col = GBA_RGB(26 - t, 21 - t, 13 - t);
        r_hline(0, y, 240, col);
    }

    /* Border frame */
    r_fill(0, 0, 240, 160, RC_GOLD_D);
    r_fill(3, 3, 234, 154, GBA_RGB(26, 21, 13));
    r_hline(3, 3, 234, RC_GOLD_L);
    r_vline(3, 3, 154, RC_GOLD_L);

    /* Title */
    r_fill(50, 3, 140, 15, GBA_RGB(10,6,2));
    r_hline(50, 3, 140, RC_GOLD);
    r_hline(50, 17, 140, RC_GOLD);
    r_str_c(120, 6, "HEARTHWOOD MAP", RC_GOLD_L, GBA_RGB(10,6,2));

    /* Six named regions arranged roughly like the world layout:
     *   OAK GLADE (top-left)   |  open meadow (top-centre)  | MARROW CAVE (top-right)
     *   PONDSHORE (mid-left)   |  WILLOW HOVEL (mid-centre) | DRIFTWOOD (mid-right)
     *   crossroads (bot-left)  |  CROSSROADS (bot-centre)   | (empty)
     */
    wmap_region( 8, 24, 66, 38, GBA_RGB( 4,12, 3), "OAK GLADE");
    wmap_region(80, 24, 80, 20, GBA_RGB( 8,13, 5), "MEADOW");
    wmap_region(166,24, 66, 38, GBA_RGB( 9, 9,13), "MARROW CAVE");

    wmap_region( 8, 68, 66, 38, GBA_RGB( 3, 7,16), "PONDSHORE");
    wmap_region(80, 50, 80, 36, GBA_RGB(11, 8, 3), "WILLOW HOVEL");
    wmap_region(166,68, 66, 38, GBA_RGB( 5,12, 3), "DRIFTWOOD");

    wmap_region(40,112, 160,18, GBA_RGB(14,10, 5), "CROSSROADS PATH");

    /* Hovel level badge */
    {
        char badge[24];
        const char *pre = "HOVEL: ";
        int bi = 0;
        for (int i = 0; pre[i]; i++) badge[bi++] = pre[i];
        const char *sn = hovel_stage_name(hovel_level);
        for (int i = 0; sn[i]; i++) badge[bi++] = sn[i];
        badge[bi] = '\0';
        r_str_c(120, 137, badge, RC_GOLD_L, GBA_RGB(26,21,13));
    }

    r_str_c(120, 149, "PRESS ANY BUTTON", RC_CREAM, GBA_RGB(26,21,13));
}

/* ── Upgrade prompt on results screen ───────────────────────────────────── */

/* Append integer n to buf[*ci] (no leading zeros suppression). */
static void upg_append_int(char *buf, int *ci, int n)
{
    char tmp[8]; int ti = 0;
    if (n <= 0) { buf[(*ci)++] = '0'; return; }
    while (n > 0) { tmp[ti++] = (char)('0' + n % 10); n /= 10; }
    for (int d = ti-1; d >= 0; d--) buf[(*ci)++] = tmp[d];
}

static void draw_upgrade_row(bool can_upgrade)
{
    int strip_y = 131;
    u16 bg = GBA_RGB(5,3,0);
    r_fill(24, strip_y, 192, 14, bg);
    r_hline(24, strip_y, 192, RC_GOLD_D);

    if (hovel_level >= HOVEL_MAX_LEVEL) {
        r_str_c(120, strip_y+4, "HOVEL: MANOR COMPLETE!", RC_GOLD_L, bg);
        return;
    }

    /* Build "B:UPGRADE (W:20 G:15)" style string */
    char cost[48]; int ci = 0;
    const char *p0 = "B:UPGRADE (W:";
    for (int i = 0; p0[i]; i++) cost[ci++] = p0[i];
    upg_append_int(cost, &ci, hovel_wood_cost[hovel_level]);
    if (hovel_gold_cost[hovel_level] > 0) {
        const char *p1 = " G:";
        for (int i=0;p1[i];i++) cost[ci++]=p1[i];
        upg_append_int(cost, &ci, hovel_gold_cost[hovel_level]);
    }
    if (hovel_fish_cost[hovel_level] > 0) {
        const char *p2 = " F:";
        for (int i=0;p2[i];i++) cost[ci++]=p2[i];
        upg_append_int(cost, &ci, hovel_fish_cost[hovel_level]);
    }
    cost[ci++] = ')'; cost[ci] = '\0';

    u16 tcol = can_upgrade ? RC_GOLD_L : GBA_RGB(12,8,3);
    r_str_c(120, strip_y+4, cost, tcol, bg);
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void results_init(int day, int wood_today, int gold_today, int fish_today, int streak)
{
    rs_day    = day;
    rs_wood   = wood_today;
    rs_gold   = gold_today;
    rs_fish   = fish_today;
    rs_streak = streak;
    rs_timer  = 0;

    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    /* Hide all OAM sprites while in Mode 3 */
    for (int i = 0; i < 128; i++) {
        OAM_MEM[i].attr0 = OBJ_HIDE;
        OAM_MEM[i].attr1 = 0;
        OAM_MEM[i].attr2 = 0;
    }

    draw_full_results(true);
    draw_upgrade_row(hovel_can_upgrade());
}

bool results_update(void)
{
    rs_timer++;

    u16 keys = REG_KEYINPUT;
    static u16 prev_keys = 0x03FF;

    bool a_press   = ((prev_keys & KEY_A)      != 0) && ((keys & KEY_A)      == 0);
    bool b_press   = ((prev_keys & KEY_B)      != 0) && ((keys & KEY_B)      == 0);
    bool sel_press = ((prev_keys & KEY_SELECT)  != 0) && ((keys & KEY_SELECT) == 0);
    prev_keys = keys;

    /* B: hovel upgrade */
    if (b_press && hovel_can_upgrade()) {
        wood -= hovel_wood_cost[hovel_level];
        gold -= hovel_gold_cost[hovel_level];
        food -= hovel_fish_cost[hovel_level];
        hovel_level++;
        /* Redraw results (now showing updated hovel name in panel) */
        draw_full_results(true);
        draw_upgrade_row(hovel_can_upgrade());
    }

    /* SELECT: world map overlay */
    if (sel_press) {
        draw_world_map();
        /* Wait for any button */
        u16 prev_w = REG_KEYINPUT;
        while (true) {
            vblank_wait();
            u16 cur_w = REG_KEYINPUT;
            /* any button released */
            if ((prev_w ^ cur_w) & ~cur_w) break;
            prev_w = cur_w;
        }
        /* Restore results screen */
        draw_full_results(true);
        draw_upgrade_row(hovel_can_upgrade());
    }

    /* Auto-advance */
    if (rs_timer >= RESULT_TIMEOUT)
        return true;

    /* Blink combined prompt every ~30 frames */
    bool blink = ((rs_timer / 30) & 1) == 0;
    u16 pc = blink ? RC_CREAM : RC_PARCH;
    r_str_c(120, 148, "A:REST  SELECT:MAP", pc, RC_GRASSD);

    return a_press;
}
