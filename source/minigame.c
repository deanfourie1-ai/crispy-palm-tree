/*
 * minigame.c  –  Gathering mini-game overlay system.
 *
 * Renders a "battle initiate" style overlay using GBA Mode 3 (240×160
 * bitmap) whenever a peasant arrives at a resource tile.
 *
 * ── Phase flow ────────────────────────────────────────────────────────────
 *
 *   MG_INACTIVE
 *       │  minigame_start()
 *       ▼
 *   MG_ACTIVE
 *       Stable overlay (no transition redraw).  Placeholder interaction:
 *       blink "PRESS A", timeout safety net.
 *       │  A pressed  OR  ACTIVE_TIMEOUT frames elapsed
 *       ▼
 *   MG_DONE
 *       minigame_get_yield() is valid.  Caller reads yield, calls
 *       minigame_reset() → returns to MG_INACTIVE.
 *
 * ── Display switching ─────────────────────────────────────────────────────
 *
 *   minigame_start()   : DISPCNT → Mode 3 | BG2  (no OBJ layer)
 *   MG_DONE transition : DISPCNT → Mode 0 | BG0 | OBJ | OBJ_1D
 *
 * ── Themed backdrops ──────────────────────────────────────────────────────
 *
 *   GATHER_WOOD  →  Forest green background, tree silhouettes, "LUMBER" banner
 *   GATHER_FISH  →  Ocean blue background,  wave lines + fish,  "FISHING" banner
 *   GATHER_ORE   →  Cave grey background,   stalactites + crystal,"MINING" banner
 *
 * ── Performance note ──────────────────────────────────────────────────────
 *
 *   The backdrop is drawn once on entry.  Active updates only redraw the
 *   prompt area (< 100 writes per frame), avoiding transition-time tearing
 *   and shimmer on hardware/emulator timing variance.
 */

#include "minigame.h"
#include "unit.h"    /* GATHER_WOOD / GATHER_FISH / GATHER_ORE */

/* ── Mode 3 pixel helpers ─────────────────────────────────────────────────*/
#define M3_VRAM       ((volatile u16 *)0x06000000)
#define M3_PIX(x, y)  M3_VRAM[(y) * 240 + (x)]

/* ── Timing constants ─────────────────────────────────────────────────────*/
#define ACTIVE_TIMEOUT 300  /* fallback: auto-complete after ~5 seconds   */

/* ── Colour palette ───────────────────────────────────────────────────────*/
#define C_BLACK        GBA_RGB( 0,  0,  0)
#define C_WHITE        GBA_RGB(31, 31, 31)

/* Wood theme */
#define C_FOREST_BG    GBA_RGB( 2,  8,  2)
#define C_FOREST_MID   GBA_RGB( 3, 12,  3)
#define C_BARK         GBA_RGB(10,  7,  2)
#define C_LEAF         GBA_RGB( 5, 18,  5)
#define C_LEAF_HI      GBA_RGB( 8, 26,  8)

/* Fish theme */
#define C_OCEAN_BG     GBA_RGB( 1,  4, 14)
#define C_OCEAN_MID    GBA_RGB( 2,  8, 18)
#define C_FOAM         GBA_RGB(22, 26, 30)
#define C_FOAM_DIM     GBA_RGB(10, 14, 18)
#define C_FISH_COL     GBA_RGB(28, 18,  4)

/* Ore theme */
#define C_CAVE_BG      GBA_RGB( 4,  4,  5)
#define C_CAVE_MID     GBA_RGB( 7,  7,  8)
#define C_STONE        GBA_RGB(14, 13, 12)
#define C_ORE_GLOW     GBA_RGB(28, 14,  2)
#define C_ORE_HI       GBA_RGB(31, 28, 18)
#define C_ORE_DARK     GBA_RGB(18,  8,  1)

/* Shared accent */
#define C_GOLD         GBA_RGB(28, 22,  4)
#define C_GOLD_BRT     GBA_RGB(31, 29, 12)
#define C_SHADOW       GBA_RGB( 4,  3,  1)

/* ── Minimal 5×7 bitmap font ──────────────────────────────────────────────*/
/* Each row byte: bit 4 = leftmost pixel, bit 0 = rightmost.               */
#define FONT_W 5
#define FONT_H 7

static const struct { char c; u8 r[7]; } mg_font[] = {
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
    {'!',{0x04,0x04,0x04,0x04,0x04,0x00,0x04}},
    {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3',{0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}},
    {'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5',{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
};
#define MG_FONT_COUNT ((int)(sizeof(mg_font) / sizeof(mg_font[0])))

/* ── Module state ──────────────────────────────────────────────────────────*/
typedef enum {
    MG_INACTIVE = 0,
    MG_ACTIVE,
    MG_DONE
} MgPhase;

static MgPhase mg_phase  = MG_INACTIVE;
static u8      mg_type   = GATHER_WOOD;
static int     mg_timer  = 0;
static u8      mg_score  = 0;   /* 0–4 performance score */
static u16     mg_prev_keys = 0x03FF;
static u32     mg_rng = 0xA5C3F217;

typedef enum {
    FISH_CAST_READY = 0,
    FISH_WAIT_BITE,
    FISH_BITE_WINDOW,
    FISH_RESULT
} FishPhase;

static FishPhase fish_phase      = FISH_CAST_READY;
static FishPhase fish_last_phase = (FishPhase)0xFF; /* force bg draw on first frame */
static int  fish_timer       = 0;
static int  fish_bite_delay  = 0;
static bool fish_success     = false;
static int  fish_input_lock  = 0;
static int  fish_prev_bobber_y    = -1;
static bool fish_prev_bite_flash  = false;

typedef enum {
    WOOD_READY = 0,
    WOOD_CHOPPING,
    WOOD_RESULT
} WoodPhase;

static WoodPhase wood_phase      = WOOD_READY;
static WoodPhase wood_last_phase = (WoodPhase)0xFF; /* force bg draw on first frame */
static int  wood_timer           = 0;
static int  wood_hits            = 0;
static int  wood_quality         = 0;
static int  wood_bar_x           = 0;
static int  wood_bar_dir         = 1;
static int  wood_sweet_x         = 0;
static int  wood_sweet_w         = 0;
static bool wood_success         = false;
static int  wood_prev_bar_x      = -1;

typedef enum {
    ORE_READY = 0,
    ORE_STRIKING,
    ORE_RESULT
} OrePhase;

static OrePhase ore_phase         = ORE_READY;
static OrePhase ore_last_phase    = (OrePhase)0xFF; /* force bg draw on first frame */
static int      ore_timer         = 0;
static int      ore_hits          = 0;
static int      ore_quality       = 0;
static int      ore_cursor_x      = 0;
static int      ore_cursor_dir    = 1;
static int      ore_spot_x[3]     = {0, 0, 0};
static int      ore_spot_w[3]     = {0, 0, 0};
static bool     ore_spot_done[3]  = {false, false, false};
static bool     ore_success       = false;
static int      ore_prev_cursor_x = -1;

#define FISH_BOX_X 108
#define FISH_BOX_Y 44
#define FISH_BOX_W 128
#define FISH_BOX_H 108

#define WOOD_BOX_X FISH_BOX_X
#define WOOD_BOX_Y FISH_BOX_Y
#define WOOD_BOX_W FISH_BOX_W
#define WOOD_BOX_H FISH_BOX_H

#define WOOD_TRACK_X0 (WOOD_BOX_X + 10)
#define WOOD_TRACK_X1 (WOOD_BOX_X + WOOD_BOX_W - 11)
#define WOOD_TRACK_Y0 (WOOD_BOX_Y + 58)
#define WOOD_TRACK_Y1 (WOOD_BOX_Y + 78)

#define ORE_BOX_X WOOD_BOX_X
#define ORE_BOX_Y WOOD_BOX_Y
#define ORE_BOX_W WOOD_BOX_W
#define ORE_BOX_H WOOD_BOX_H

#define ORE_ARC_X0 (ORE_BOX_X + 12)
#define ORE_ARC_X1 (ORE_BOX_X + ORE_BOX_W - 13)
#define ORE_ARC_Y0 (ORE_BOX_Y + 56)
#define ORE_ARC_Y1 (ORE_BOX_Y + 80)

/* ── Drawing primitives ────────────────────────────────────────────────────*/
static void mg_fill(int x, int y, int w, int h, u16 col)
{
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            M3_PIX(i, j) = col;
}

static void mg_hline(int x, int y, int w, u16 col)
{
    for (int i = x; i < x + w; i++) M3_PIX(i, y) = col;
}

static void mg_vline(int x, int y, int h, u16 col)
{
    for (int j = y; j < y + h; j++) M3_PIX(x, j) = col;
}

static void mg_draw_char(int x, int y, char ch, u16 fg, u16 bg)
{
    for (int i = 0; i < MG_FONT_COUNT; i++) {
        if (mg_font[i].c != ch) continue;
        for (int row = 0; row < FONT_H; row++) {
            u8 bits = mg_font[i].r[row];
            for (int col = 0; col < FONT_W; col++) {
                int px = x + col, py = y + row;
                if (px >= 0 && px < 240 && py >= 0 && py < 160)
                    M3_PIX(px, py) = (bits & (0x10 >> col)) ? fg : bg;
            }
        }
        return;
    }
}

static void mg_draw_str(int x, int y, const char *s, u16 fg, u16 bg)
{
    for (; *s; s++, x += FONT_W + 1)
        mg_draw_char(x, y, *s, fg, bg);
}

static int mg_str_w(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    return n > 0 ? n * (FONT_W + 1) - 1 : 0;
}

/* Draw string centred horizontally at row y with a 1-pixel drop shadow. */
static void mg_draw_str_centred(int y, const char *s, u16 fg, u16 bg)
{
    int x = (240 - mg_str_w(s)) / 2;
    mg_draw_str(x + 1, y + 1, s, C_SHADOW, bg);
    mg_draw_str(x,     y,     s, fg,        bg);
}

static u32 mg_rand_next(void)
{
    /* Simple deterministic LCG; good enough for timing variation. */
    mg_rng = mg_rng * 1664525u + 1013904223u;
    return mg_rng;
}

/* ── Icon painters ─────────────────────────────────────────────────────────*/

/* Pine tree silhouette anchored at (cx, cy), ~12 px wide × 15 px tall. */
static void draw_tree(int cx, int cy)
{
    mg_fill(cx + 4, cy + 10, 4, 5, C_BARK);          /* trunk          */
    mg_fill(cx + 2, cy +  6, 8, 5, C_LEAF);           /* lower canopy   */
    mg_fill(cx + 1, cy +  2, 10, 5, C_LEAF);          /* mid canopy     */
    mg_fill(cx + 3, cy,      6, 3, C_LEAF);            /* top canopy     */
    M3_PIX(cx + 2, cy + 2) = C_LEAF_HI;               /* highlight      */
    M3_PIX(cx + 4, cy)     = C_LEAF_HI;
}

/* Simple fish body anchored at (cx, cy), ~12 px wide × 8 px tall. */
static void draw_fish(int cx, int cy)
{
    /* Tail */
    M3_PIX(cx,     cy + 1) = C_FISH_COL;
    M3_PIX(cx + 1, cy)     = C_FISH_COL;
    M3_PIX(cx + 1, cy + 4) = C_FISH_COL;
    M3_PIX(cx,     cy + 5) = C_FISH_COL;
    /* Body */
    mg_fill(cx + 2, cy + 1, 8, 5, C_FISH_COL);
    /* Fin */
    mg_hline(cx + 4, cy,     3, C_FISH_COL);
    /* Eye */
    M3_PIX(cx + 8, cy + 3) = C_WHITE;
    /* Belly highlight */
    mg_hline(cx + 3, cy + 5, 5, GBA_RGB(31, 26, 12));
}

/* Ore crystal anchored at (cx, cy), ~10 px wide × 12 px tall. */
static void draw_crystal(int cx, int cy)
{
    mg_fill(cx + 2, cy,      6, 2,  C_ORE_GLOW);      /* top tip        */
    mg_fill(cx,     cy + 2,  10, 6, C_ORE_GLOW);      /* wide body      */
    mg_fill(cx + 2, cy + 8,  6, 2,  C_ORE_GLOW);      /* bottom tip     */
    M3_PIX(cx + 3, cy + 1) = C_ORE_HI;                /* highlight      */
    M3_PIX(cx + 2, cy + 3) = C_ORE_HI;
    mg_fill(cx + 6, cy + 2,  4, 6,  C_ORE_DARK);      /* shadow facet   */
}

/* ── Backdrop painters ─────────────────────────────────────────────────────*/

/*
 * Each backdrop is:
 *   • A full 240×160 fill (background field)
 *   • A 24-px tall title banner at the top
 *   • A vertical divider at x=100 separating left icon panel / right action area
 *   • Themed decorations in the left panel
 *   • A 1-px border around the screen edges
 *
 * The right panel (x 104–236) is the mini-game interaction area.
 * A "READY" label and later the per-type game element are drawn there.
 */

static void draw_wood_backdrop(void)
{
    /* Background: alternating dark-green horizontal bands */
    for (int y = 0; y < 160; y++)
        mg_hline(0, y, 240, (y & 2) ? C_FOREST_MID : C_FOREST_BG);

    /* Title banner */
    mg_fill(0, 0, 240, 24, C_BARK);
    mg_hline(0, 24, 240, GBA_RGB(14, 10, 3));
    mg_hline(0, 25, 240, GBA_RGB( 8,  6, 1));
    mg_draw_str_centred(8, "LUMBER", C_GOLD_BRT, C_BARK);

    /* Vertical divider */
    mg_vline(100, 26, 134, C_BARK);
    mg_vline(101, 26, 134, GBA_RGB(6, 4, 1));

    /* Tree icons in left panel */
    draw_tree(12, 50);
    draw_tree(38, 42);
    draw_tree(62, 54);

    mg_draw_str(108, 30, "SWING TRUE", C_GOLD_BRT, C_FOREST_BG);
    mg_hline(108, 40, 128, C_BARK);

    /* Outer border */
    mg_hline(0,   0,   240, C_GOLD);
    mg_hline(0,   159, 240, C_GOLD);
    mg_vline(0,   0,   160, C_GOLD);
    mg_vline(239, 0,   160, C_GOLD);
}

static void draw_fish_backdrop(void)
{
    /* Background: ocean bands */
    for (int y = 0; y < 160; y++) {
        int band = y / 4;
        mg_hline(0, y, 240, (band & 1) ? C_OCEAN_MID : C_OCEAN_BG);
    }

    /* Foam wave lines */
    for (int x = 0; x < 240; x += 3) {
        int wy = 90 + (x % 7);
        if (wy < 160) M3_PIX(x, wy) = C_FOAM;
        wy = 112 + ((x / 2) % 5);
        if (wy < 160) M3_PIX(x, wy) = C_FOAM;
    }

    /* Title banner */
    mg_fill(0, 0, 240, 24, C_OCEAN_BG);
    mg_hline(0, 24, 240, C_FOAM);
    mg_hline(0, 25, 240, C_FOAM_DIM);
    mg_draw_str_centred(8, "FISHING", C_FOAM, C_OCEAN_BG);

    /* Vertical divider */
    mg_vline(100, 26, 134, C_FOAM);
    mg_vline(101, 26, 134, C_FOAM_DIM);

    /* Fish icons in left panel */
    draw_fish(10, 58);
    draw_fish(14, 74);
    draw_fish(30, 65);

    mg_draw_str(108, 30, "WATCH AND WAIT", C_FOAM, C_OCEAN_BG);
    mg_hline(108, 40, 128, C_FOAM_DIM);

    /* Outer border */
    mg_hline(0,   0,   240, C_FOAM);
    mg_hline(0,   159, 240, C_FOAM);
    mg_vline(0,   0,   160, C_FOAM);
    mg_vline(239, 0,   160, C_FOAM);
}

static void draw_ore_backdrop(void)
{
    /* Background: cave bands */
    for (int y = 0; y < 160; y++)
        mg_hline(0, y, 240, (y & 4) ? C_CAVE_MID : C_CAVE_BG);

    /* Stalactites from top (left panel) */
    for (int x = 8; x < 96; x += 16) {
        int h = 12 + (x % 9);
        mg_fill(x, 26, 4, h, C_STONE);
        M3_PIX(x + 2, 26 + h) = C_STONE;  /* tip pixel */
    }

    /* Title banner */
    mg_fill(0, 0, 240, 24, C_CAVE_BG);
    mg_hline(0, 24, 240, C_ORE_GLOW);
    mg_hline(0, 25, 240, GBA_RGB(14, 7, 1));
    mg_draw_str_centred(8, "MINING", C_ORE_GLOW, C_CAVE_BG);

    /* Vertical divider */
    mg_vline(100, 26, 134, C_STONE);
    mg_vline(101, 26, 134, GBA_RGB(8, 8, 9));

    /* Crystal icons in left panel */
    draw_crystal(18, 60);
    draw_crystal(52, 70);

    mg_draw_str(108, 30, "HIT THE VEIN", C_ORE_GLOW, C_CAVE_BG);
    mg_hline(108, 40, 128, C_ORE_DARK);

    /* Outer border */
    mg_hline(0,   0,   240, C_ORE_GLOW);
    mg_hline(0,   159, 240, C_ORE_GLOW);
    mg_vline(0,   0,   160, C_ORE_GLOW);
    mg_vline(239, 0,   160, C_ORE_GLOW);
}

static void draw_backdrop(void)
{
    switch (mg_type) {
        case GATHER_WOOD: draw_wood_backdrop(); break;
        case GATHER_FISH: draw_fish_backdrop(); break;
        default:          draw_ore_backdrop();  break;
    }
}

/* ── Active phase scaffold ──────────────────────────────────────────────── */

/*
 * Resolve accent colours for the current resource type so the prompt
 * blends with the themed backdrop.
 */
static void active_colours(u16 *fg, u16 *bg)
{
    switch (mg_type) {
        case GATHER_WOOD: *fg = C_GOLD_BRT; *bg = C_FOREST_BG; break;
        case GATHER_FISH: *fg = C_FOAM;     *bg = C_OCEAN_BG;  break;
        default:          *fg = C_ORE_GLOW; *bg = C_CAVE_BG;   break;
    }
}

static const char *score_label(u8 score)
{
    switch (score) {
        case 4: return "PERFECT";
        case 3: return "GREAT";
        case 2: return "GOOD";
        case 1: return "OK";
        default: return "MISS";
    }
}

#define PANEL_CX   (FISH_BOX_X + FISH_BOX_W / 2)  /* 172 */
#define LINE_START (FISH_BOX_Y + 30)               /*  74 */

/* Returns the background colour that belongs at pixel (px, py) inside the
   fishing action panel, so we can restore it when erasing the bobber. */
static u16 fishing_panel_bg_at(int px, int py)
{
    int water_top = FISH_BOX_Y + 46;
    if (py < water_top) return C_OCEAN_BG;
    /* Wave dot check */
    int rel_x = px - (FISH_BOX_X + 8);
    if (rel_x >= 0 && rel_x < (FISH_BOX_W - 16) && (rel_x % 10) < 6) {
        int wave_y = water_top + ((px >> 2) & 3);
        if (wave_y == py) return C_FOAM_DIM;
    }
    return ((py >> 1) & 1) ? C_OCEAN_MID : C_OCEAN_BG;
}

/* ── STATIC draw – call once per fishing phase change (~22 K writes). ─────
   Draws background, water, waves, borders, text.  NO bobber or line. */
static void fishing_draw_panel_bg(const char *top, const char *bottom)
{
    const int x = FISH_BOX_X, y = FISH_BOX_Y;
    const int w = FISH_BOX_W, h = FISH_BOX_H;
    const int water_top = y + 46;

    mg_fill(x, y, w, h, C_OCEAN_BG);
    mg_hline(x,       y,       w, C_FOAM);
    mg_hline(x,       y+h-1,   w, C_FOAM);
    mg_vline(x,       y,       h, C_FOAM);
    mg_vline(x+w-1,   y,       h, C_FOAM);

    for (int yy = water_top; yy < y+h-1; yy++) {
        u16 row = ((yy >> 1) & 1) ? C_OCEAN_MID : C_OCEAN_BG;
        mg_hline(x+1, yy, w-2, row);
    }
    for (int wx = x+8; wx < x+w-8; wx += 10) {
        int wave_y = water_top + ((wx >> 2) & 3);
        mg_hline(wx, wave_y, 6, C_FOAM_DIM);
    }

    mg_draw_str(x+8, y+8,  top,    C_FOAM,     C_OCEAN_BG);
    mg_draw_str(x+8, y+20, bottom, C_FOAM_DIM, C_OCEAN_BG);
}

/* ── DYNAMIC erase – restores background pixels where bobber/line were. ──
   ~120 writes per call. */
static void fishing_erase_bobber(int bobber_y, bool had_flash)
{
    const int cx = PANEL_CX;
    /* Erase the line column */
    for (int ly = LINE_START; ly <= bobber_y + 8; ly++)
        M3_PIX(cx, ly) = fishing_panel_bg_at(cx, ly);
    /* Erase bobber 7×7 bounding box */
    for (int dy = -3; dy <= 4; dy++)
        for (int dx = -3; dx <= 3; dx++)
            M3_PIX(cx+dx, bobber_y+dy) = fishing_panel_bg_at(cx+dx, bobber_y+dy);
    /* Erase bite-flash indicators */
    if (had_flash) {
        for (int bx = cx-10; bx <= cx+12; bx++) {
            M3_PIX(bx, bobber_y+5) = fishing_panel_bg_at(bx, bobber_y+5);
            M3_PIX(bx, bobber_y+6) = fishing_panel_bg_at(bx, bobber_y+6);
        }
    }
}

/* ── DYNAMIC draw – bobber + line only. ~70 writes per call. */
static void fishing_draw_bobber(int bobber_y, bool bite_flash)
{
    const int cx = PANEL_CX;
    /* Line */
    for (int ly = LINE_START; ly < bobber_y - 3; ly++)
        M3_PIX(cx, ly) = C_FOAM;
    /* Bobber */
    mg_fill(cx-2, bobber_y-2, 5, 5, GBA_RGB(31, 20, 5));
    mg_hline(cx-2, bobber_y, 5, C_BLACK);
    M3_PIX(cx, bobber_y-3) = C_FOAM;
    /* Bite flash indicators */
    if (bite_flash) {
        mg_hline(cx-8, bobber_y+6, 4, C_FOAM);
        mg_hline(cx+5, bobber_y+6, 4, C_FOAM);
        M3_PIX(cx-10, bobber_y+5) = C_FOAM;
        M3_PIX(cx+10, bobber_y+5) = C_FOAM;
    }
}

/* Returns the background colour that belongs at pixel (px, py) inside the
   wood action panel, so we can restore it when erasing the marker. */
static u16 wood_panel_bg_at(int px, int py)
{
    if (px == WOOD_BOX_X || px == WOOD_BOX_X + WOOD_BOX_W - 1 ||
        py == WOOD_BOX_Y || py == WOOD_BOX_Y + WOOD_BOX_H - 1)
        return C_GOLD;

    if (py >= WOOD_TRACK_Y0 && py <= WOOD_TRACK_Y1 && px >= WOOD_TRACK_X0 && px <= WOOD_TRACK_X1) {
        if (py == WOOD_TRACK_Y0 || py == WOOD_TRACK_Y1 || px == WOOD_TRACK_X0 || px == WOOD_TRACK_X1)
            return C_BARK;

        if (px >= wood_sweet_x && px < wood_sweet_x + wood_sweet_w &&
            py >= WOOD_TRACK_Y0 + 2 && py <= WOOD_TRACK_Y1 - 2)
            return C_LEAF_HI;

        return C_FOREST_MID;
    }

    return C_FOREST_BG;
}

/* ── STATIC draw – call on wood phase/sweet-spot changes. */
static void wood_draw_panel_bg(const char *top, const char *bottom)
{
    const int x = WOOD_BOX_X, y = WOOD_BOX_Y;
    const int w = WOOD_BOX_W, h = WOOD_BOX_H;
    int sweet_label_x;
    int sweet_label_y;

    mg_fill(x, y, w, h, C_FOREST_BG);
    mg_hline(x,       y,       w, C_GOLD);
    mg_hline(x,       y+h-1,   w, C_GOLD);
    mg_vline(x,       y,       h, C_GOLD);
    mg_vline(x+w-1,   y,       h, C_GOLD);

    mg_draw_str(x+8, y+8,  top,    C_GOLD_BRT, C_FOREST_BG);
    mg_draw_str(x+8, y+20, bottom, C_GOLD,     C_FOREST_BG);

    mg_fill(WOOD_TRACK_X0, WOOD_TRACK_Y0, WOOD_TRACK_X1 - WOOD_TRACK_X0 + 1,
            WOOD_TRACK_Y1 - WOOD_TRACK_Y0 + 1, C_BARK);
    mg_fill(WOOD_TRACK_X0 + 1, WOOD_TRACK_Y0 + 1,
            WOOD_TRACK_X1 - WOOD_TRACK_X0 - 1,
            WOOD_TRACK_Y1 - WOOD_TRACK_Y0 - 1, C_FOREST_MID);

    mg_fill(wood_sweet_x, WOOD_TRACK_Y0 + 2, wood_sweet_w,
            WOOD_TRACK_Y1 - WOOD_TRACK_Y0 - 3, C_LEAF_HI);

    mg_draw_str(x+8, y+88, "HITS", C_GOLD_BRT, C_FOREST_BG);
    for (int i = 0; i < 3; i++) {
        u16 pip = (i < wood_hits) ? C_GOLD_BRT : C_BARK;
        mg_fill(x + 36 + i * 10, y + 88, 8, 6, pip);
    }

    sweet_label_x = x + w - 58;
    sweet_label_y = y + 88;
    mg_draw_str(sweet_label_x, sweet_label_y, "ZONE", C_GOLD_BRT, C_FOREST_BG);
}

/* ── DYNAMIC marker erase/draw – tiny per frame write budget. */
static void wood_erase_marker(int bar_x)
{
    for (int py = WOOD_TRACK_Y0 - 2; py <= WOOD_TRACK_Y1 + 2; py++) {
        for (int px = bar_x - 1; px <= bar_x + 1; px++)
            M3_PIX(px, py) = wood_panel_bg_at(px, py);
    }
}

static void wood_draw_marker(int bar_x)
{
    for (int py = WOOD_TRACK_Y0 - 2; py <= WOOD_TRACK_Y1 + 2; py++) {
        M3_PIX(bar_x, py) = C_WHITE;
        if ((py & 1) == 0) {
            M3_PIX(bar_x - 1, py) = C_GOLD_BRT;
            M3_PIX(bar_x + 1, py) = C_GOLD_BRT;
        }
    }
}

static bool active_update_wood(bool a_just_pressed, bool b_just_pressed)
{
    int speed;
    int sweet_cx;
    int dist;

    if (b_just_pressed && wood_phase != WOOD_RESULT) {
        wood_success = false;
        mg_score = 0;
        wood_phase = WOOD_RESULT;
        wood_timer = 0;
        wood_last_phase = (WoodPhase)0xFF;
    }

    if (wood_phase != wood_last_phase) {
        switch (wood_phase) {
            case WOOD_READY:
                wood_draw_panel_bg("RHYTHM CHOP", "PRESS A TO START");
                break;
            case WOOD_CHOPPING:
                wood_draw_panel_bg("CHOP", "PRESS A IN THE ZONE");
                break;
            case WOOD_RESULT:
                if (wood_success)
                    wood_draw_panel_bg("TIMBER", score_label(mg_score));
                else
                    wood_draw_panel_bg("WHIFF", "MISS");
                break;
            default:
                break;
        }
        wood_last_phase = wood_phase;
        wood_prev_bar_x = -1;
    }

    if (wood_phase == WOOD_CHOPPING) {
        speed = 2 + wood_hits;
        wood_bar_x += wood_bar_dir * speed;
        if (wood_bar_x <= WOOD_TRACK_X0 + 2) {
            wood_bar_x = WOOD_TRACK_X0 + 2;
            wood_bar_dir = 1;
        } else if (wood_bar_x >= WOOD_TRACK_X1 - 2) {
            wood_bar_x = WOOD_TRACK_X1 - 2;
            wood_bar_dir = -1;
        }

        if (wood_prev_bar_x >= 0)
            wood_erase_marker(wood_prev_bar_x);
        wood_draw_marker(wood_bar_x);
        wood_prev_bar_x = wood_bar_x;
    }

    switch (wood_phase) {
        case WOOD_READY:
            if (a_just_pressed) {
                wood_phase = WOOD_CHOPPING;
                wood_timer = 0;
            }
            break;

        case WOOD_CHOPPING:
            if (a_just_pressed) {
                sweet_cx = wood_sweet_x + (wood_sweet_w / 2);
                dist = wood_bar_x - sweet_cx;
                if (dist < 0) dist = -dist;

                if (dist <= (wood_sweet_w / 2)) {
                    wood_hits++;
                    if (dist <= 2)      wood_quality += 3;
                    else if (dist <= 5) wood_quality += 2;
                    else                wood_quality += 1;

                    if (wood_hits >= 3) {
                        wood_success = true;
                        if      (wood_quality >= 8) mg_score = 4;
                        else if (wood_quality >= 6) mg_score = 3;
                        else if (wood_quality >= 4) mg_score = 2;
                        else                        mg_score = 1;
                        wood_phase = WOOD_RESULT;
                        wood_timer = 0;
                    } else {
                        int max_w = (WOOD_TRACK_X1 - WOOD_TRACK_X0 - 10);
                        wood_sweet_w -= 6;
                        if (wood_sweet_w < 12) wood_sweet_w = 12;
                        wood_sweet_x = WOOD_TRACK_X0 + 6 + (int)(mg_rand_next() % (u32)max_w);
                        if (wood_sweet_x + wood_sweet_w > WOOD_TRACK_X1 - 4)
                            wood_sweet_x = WOOD_TRACK_X1 - 4 - wood_sweet_w;
                        wood_draw_panel_bg("CHOP", "PRESS A IN THE ZONE");
                        wood_prev_bar_x = -1;
                    }
                } else {
                    wood_success = false;
                    mg_score = 0;
                    wood_phase = WOOD_RESULT;
                    wood_timer = 0;
                }
            }

            wood_timer++;
            if (wood_timer >= 240) {
                wood_success = false;
                mg_score = 0;
                wood_phase = WOOD_RESULT;
                wood_timer = 0;
            }
            break;

        case WOOD_RESULT:
            wood_timer++;
            if (wood_timer >= 48)
                return true;
            break;

        default:
            wood_phase = WOOD_READY;
            wood_timer = 0;
            break;
    }

    return false;
}

/* Returns the background colour that belongs at pixel (px, py) inside the
   ore action panel, so we can restore it when erasing the cursor. */
static u16 ore_panel_bg_at(int px, int py)
{
    if (px == ORE_BOX_X || px == ORE_BOX_X + ORE_BOX_W - 1 ||
        py == ORE_BOX_Y || py == ORE_BOX_Y + ORE_BOX_H - 1)
        return C_ORE_GLOW;

    if (py >= ORE_ARC_Y0 && py <= ORE_ARC_Y1 && px >= ORE_ARC_X0 && px <= ORE_ARC_X1) {
        if (py == ORE_ARC_Y0 || py == ORE_ARC_Y1 || px == ORE_ARC_X0 || px == ORE_ARC_X1)
            return C_STONE;

        for (int i = 0; i < 3; i++) {
            if (!ore_spot_done[i] && px >= ore_spot_x[i] && px < ore_spot_x[i] + ore_spot_w[i] &&
                py >= ORE_ARC_Y0 + 2 && py <= ORE_ARC_Y1 - 2)
                return C_ORE_HI;
        }

        return C_CAVE_MID;
    }

    return C_CAVE_BG;
}

/* ── STATIC draw – call on ore phase/spot changes. */
static void ore_draw_panel_bg(const char *top, const char *bottom)
{
    const int x = ORE_BOX_X, y = ORE_BOX_Y;
    const int w = ORE_BOX_W, h = ORE_BOX_H;
    int i;

    mg_fill(x, y, w, h, C_CAVE_BG);
    mg_hline(x,       y,       w, C_ORE_GLOW);
    mg_hline(x,       y+h-1,   w, C_ORE_GLOW);
    mg_vline(x,       y,       h, C_ORE_GLOW);
    mg_vline(x+w-1,   y,       h, C_ORE_GLOW);

    mg_draw_str(x+8, y+8,  top,    C_ORE_HI,   C_CAVE_BG);
    mg_draw_str(x+8, y+20, bottom, C_ORE_GLOW, C_CAVE_BG);

    mg_fill(ORE_ARC_X0, ORE_ARC_Y0, ORE_ARC_X1 - ORE_ARC_X0 + 1,
            ORE_ARC_Y1 - ORE_ARC_Y0 + 1, C_STONE);
    mg_fill(ORE_ARC_X0 + 1, ORE_ARC_Y0 + 1,
            ORE_ARC_X1 - ORE_ARC_X0 - 1,
            ORE_ARC_Y1 - ORE_ARC_Y0 - 1, C_CAVE_MID);

    for (i = 0; i < 3; i++) {
        u16 spot_col = ore_spot_done[i] ? C_ORE_DARK : C_ORE_HI;
        mg_fill(ore_spot_x[i], ORE_ARC_Y0 + 2, ore_spot_w[i],
                ORE_ARC_Y1 - ORE_ARC_Y0 - 3, spot_col);
    }

    mg_draw_str(x+8, y+88, "VEINS", C_ORE_HI, C_CAVE_BG);
    for (i = 0; i < 3; i++) {
        u16 pip = ore_spot_done[i] ? C_ORE_HI : C_STONE;
        mg_fill(x + 40 + i * 10, y + 88, 8, 6, pip);
    }
}

/* ── DYNAMIC cursor erase/draw – tiny per frame write budget. */
static void ore_erase_cursor(int cx)
{
    for (int py = ORE_ARC_Y0 - 2; py <= ORE_ARC_Y1 + 2; py++) {
        for (int px = cx - 1; px <= cx + 1; px++)
            M3_PIX(px, py) = ore_panel_bg_at(px, py);
    }
}

static void ore_draw_cursor(int cx)
{
    for (int py = ORE_ARC_Y0 - 2; py <= ORE_ARC_Y1 + 2; py++) {
        M3_PIX(cx, py) = C_WHITE;
        if ((py & 1) == 0) {
            M3_PIX(cx - 1, py) = C_ORE_GLOW;
            M3_PIX(cx + 1, py) = C_ORE_GLOW;
        }
    }
}

static bool active_update_ore(bool a_just_pressed, bool b_just_pressed)
{
    int speed;
    int best_idx;
    int best_dist;

    if (b_just_pressed && ore_phase != ORE_RESULT) {
        ore_success = false;
        mg_score = 0;
        ore_phase = ORE_RESULT;
        ore_timer = 0;
        ore_last_phase = (OrePhase)0xFF;
    }

    if (ore_phase != ore_last_phase) {
        switch (ore_phase) {
            case ORE_READY:
                ore_draw_panel_bg("VEIN STRIKE", "PRESS A TO START");
                break;
            case ORE_STRIKING:
                ore_draw_panel_bg("STRIKE", "HIT BRIGHT VEINS");
                break;
            case ORE_RESULT:
                if (ore_success)
                    ore_draw_panel_bg("ORE SHATTERED", score_label(mg_score));
                else
                    ore_draw_panel_bg("GLANCING BLOW", "MISS");
                break;
            default:
                break;
        }
        ore_last_phase = ore_phase;
        ore_prev_cursor_x = -1;
    }

    if (ore_phase == ORE_STRIKING) {
        speed = 2 + ore_hits;
        ore_cursor_x += ore_cursor_dir * speed;
        if (ore_cursor_x <= ORE_ARC_X0 + 2) {
            ore_cursor_x = ORE_ARC_X0 + 2;
            ore_cursor_dir = 1;
        } else if (ore_cursor_x >= ORE_ARC_X1 - 2) {
            ore_cursor_x = ORE_ARC_X1 - 2;
            ore_cursor_dir = -1;
        }

        if (ore_prev_cursor_x >= 0)
            ore_erase_cursor(ore_prev_cursor_x);
        ore_draw_cursor(ore_cursor_x);
        ore_prev_cursor_x = ore_cursor_x;
    }

    switch (ore_phase) {
        case ORE_READY:
            if (a_just_pressed) {
                ore_phase = ORE_STRIKING;
                ore_timer = 0;
            }
            break;

        case ORE_STRIKING:
            if (a_just_pressed) {
                best_idx = -1;
                best_dist = 9999;

                for (int i = 0; i < 3; i++) {
                    int center;
                    int dist;
                    if (ore_spot_done[i]) continue;
                    center = ore_spot_x[i] + (ore_spot_w[i] / 2);
                    dist = ore_cursor_x - center;
                    if (dist < 0) dist = -dist;
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_idx = i;
                    }
                }

                if (best_idx >= 0 && best_dist <= (ore_spot_w[best_idx] / 2)) {
                    ore_spot_done[best_idx] = true;
                    ore_hits++;

                    if (best_dist <= 2)      ore_quality += 3;
                    else if (best_dist <= 5) ore_quality += 2;
                    else                     ore_quality += 1;

                    ore_draw_panel_bg("STRIKE", "HIT BRIGHT VEINS");
                    ore_prev_cursor_x = -1;

                    if (ore_hits >= 3) {
                        ore_success = true;
                        if      (ore_quality >= 8) mg_score = 4;
                        else if (ore_quality >= 6) mg_score = 3;
                        else if (ore_quality >= 4) mg_score = 2;
                        else                       mg_score = 1;
                        ore_phase = ORE_RESULT;
                        ore_timer = 0;
                    }
                } else {
                    ore_success = false;
                    mg_score = 0;
                    ore_phase = ORE_RESULT;
                    ore_timer = 0;
                }
            }

            ore_timer++;
            if (ore_timer >= 260) {
                ore_success = false;
                mg_score = 0;
                ore_phase = ORE_RESULT;
                ore_timer = 0;
            }
            break;

        case ORE_RESULT:
            ore_timer++;
            if (ore_timer >= 48)
                return true;
            break;

        default:
            ore_phase = ORE_READY;
            ore_timer = 0;
            break;
    }

    return false;
}

static bool active_update_fishing(bool a_just_pressed, bool b_just_pressed)
{
    int base_bobber_y = FISH_BOX_Y + 62;
    int bobber_y = base_bobber_y;

    if (fish_phase == FISH_WAIT_BITE)
        bobber_y += ((fish_timer >> 2) & 1) ? 3 : -3;
    else if (fish_phase == FISH_BITE_WINDOW)
        bobber_y += 8;

    if (fish_input_lock > 0) fish_input_lock--;

    /* ── B: cancel at any point ────────────────────────────────────────── */
    if (b_just_pressed && fish_phase != FISH_RESULT) {
        fish_success = false;
        mg_score = 0;
        fish_phase = FISH_RESULT;
        fish_timer = 0;
        fish_last_phase = (FishPhase)0xFF; /* force bg redraw for result screen */
    }

    /* ── Redraw static background when phase changes ─────────────────── */
    if (fish_phase != fish_last_phase) {
        /* Erase old dynamic elements before replacing the background. */
        if (fish_prev_bobber_y >= 0)
            fishing_erase_bobber(fish_prev_bobber_y, fish_prev_bite_flash);

        switch (fish_phase) {
            case FISH_CAST_READY: fishing_draw_panel_bg("FISHING",   "PRESS A TO CAST");  break;
            case FISH_WAIT_BITE:  fishing_draw_panel_bg("WAIT",      "DO NOT PRESS");      break;
            case FISH_BITE_WINDOW:fishing_draw_panel_bg("BITE!",     "PRESS A NOW");       break;
            case FISH_RESULT:
                if (fish_success)
                    fishing_draw_panel_bg("HOOKED", score_label(mg_score));
                else
                    fishing_draw_panel_bg("THE FISH GOT AWAY", "MISS");
                break;
            default: break;
        }
        fish_last_phase = fish_phase;
        fish_prev_bobber_y = -1; /* no previous bobber to erase yet */
    }

    /* ── Per-frame dynamic update: erase old bobber, draw new one ─────── */
    bool bite_flash = (fish_phase == FISH_BITE_WINDOW) && ((fish_timer & 2) == 0);
    bool bobber_moved = (bobber_y != fish_prev_bobber_y) || (bite_flash != fish_prev_bite_flash);

    if (bobber_moved && fish_prev_bobber_y >= 0)
        fishing_erase_bobber(fish_prev_bobber_y, fish_prev_bite_flash);

    if (fish_phase != FISH_RESULT) {
        fishing_draw_bobber(bobber_y, bite_flash);
        fish_prev_bobber_y   = bobber_y;
        fish_prev_bite_flash = bite_flash;
    }

    /* ── State logic ───────────────────────────────────────────────────── */
    switch (fish_phase) {
        case FISH_CAST_READY:
            if (a_just_pressed) {
                fish_phase      = FISH_WAIT_BITE;
                fish_timer      = 0;
                fish_input_lock = 12;
                fish_bite_delay = 24 + (int)(mg_rand_next() % 41u);
            }
            break;

        case FISH_WAIT_BITE:
            if (a_just_pressed && fish_input_lock == 0) {
                fish_success = false;
                mg_score     = 0;
                fish_phase   = FISH_RESULT;
                fish_timer   = 0;
            } else {
                fish_timer++;
                if (fish_timer >= fish_bite_delay) {
                    fish_phase = FISH_BITE_WINDOW;
                    fish_timer = 0;
                }
            }
            break;

        case FISH_BITE_WINDOW:
            if (a_just_pressed) {
                fish_success = true;
                if      (fish_timer <= 3)  mg_score = 4;
                else if (fish_timer <= 7)  mg_score = 3;
                else if (fish_timer <= 13) mg_score = 2;
                else                       mg_score = 1;
                fish_phase = FISH_RESULT;
                fish_timer = 0;
            } else {
                fish_timer++;
                if (fish_timer > 24) {
                    fish_success = false;
                    mg_score     = 0;
                    fish_phase   = FISH_RESULT;
                    fish_timer   = 0;
                }
            }
            break;

        case FISH_RESULT:
            fish_timer++;
            if (fish_timer >= 48)
                return true;
            break;

        default:
            fish_phase      = FISH_CAST_READY;
            fish_timer      = 0;
                fish_input_lock = 0;
                break;
    }

    return false;
}

/*
 * active_update  –  Placeholder for the real mini-game logic.
 *
 * Blinks "PRESS A" in the action area.  Pressing A scores 3 (great);
 * the ACTIVE_TIMEOUT safety net scores 1 (ok) so auto-gather never stalls.
 *
 * TODO: Replace this body with per-type mini-game implementations:
 *   GATHER_WOOD  →  Rhythm Chop  (swing-bar timing)
 *   GATHER_FISH  →  Bobber Watch (reaction tap)
 *   GATHER_ORE   →  Vein Strike  (oscillating cursor alignment)
 *
 * Returns true when the mini-game is complete; mg_score is set first.
 */
static bool active_update(void)
{
    u16 keys = REG_KEYINPUT;
    bool a_just_pressed = ((mg_prev_keys & KEY_A) != 0) && ((keys & KEY_A) == 0);
    bool b_just_pressed = ((mg_prev_keys & KEY_B) != 0) && ((keys & KEY_B) == 0);
    mg_prev_keys = keys;

    if (mg_type == GATHER_WOOD) {
        if (active_update_wood(a_just_pressed, b_just_pressed)) {
            return true;
        }

        if (mg_timer >= ACTIVE_TIMEOUT) {
            mg_score = 1;
            return true;
        }
        mg_timer++;
        return false;
    }

    if (mg_type == GATHER_FISH) {
        if (active_update_fishing(a_just_pressed, b_just_pressed)) {
            return true;
        }

        if (mg_timer >= ACTIVE_TIMEOUT) {
            mg_score = 1;
            return true;
        }
        mg_timer++;
        return false;
    }

    if (mg_type == GATHER_ORE) {
        if (active_update_ore(a_just_pressed, b_just_pressed)) {
            return true;
        }

        if (mg_timer >= ACTIVE_TIMEOUT) {
            mg_score = 1;
            return true;
        }
        mg_timer++;
        return false;
    }

    u16 fg, bg;
    active_colours(&fg, &bg);

    /* Blink "PRESS A" in the horizontal centre of the action area. */
    const char *lbl  = "PRESS A";
    int          lx  = 108 + (128 - mg_str_w(lbl)) / 2;
    int          ly  = 85;
    u16          col = ((mg_timer & 32) == 0) ? fg : bg;
    mg_draw_str(lx, ly, lbl, col, bg);

    if (a_just_pressed) {
        mg_score = 3;
        return true;
    }

    if (mg_timer >= ACTIVE_TIMEOUT) {
        mg_score = 1;
        return true;
    }

    mg_timer++;
    return false;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void minigame_start(u8 gather_type)
{
    mg_type  = gather_type;
    mg_phase = MG_ACTIVE;
    mg_timer = 0;
    mg_score = 0;
    mg_prev_keys = REG_KEYINPUT;
    fish_phase          = FISH_CAST_READY;
    fish_last_phase     = (FishPhase)0xFF;
    fish_timer          = 0;
    fish_bite_delay     = 0;
    fish_success        = false;
    fish_input_lock     = 0;
    fish_prev_bobber_y  = -1;
    fish_prev_bite_flash = false;
    wood_phase          = WOOD_READY;
    wood_last_phase     = (WoodPhase)0xFF;
    wood_timer          = 0;
    wood_hits           = 0;
    wood_quality        = 0;
    wood_bar_x          = WOOD_TRACK_X0 + 2;
    wood_bar_dir        = 1;
    wood_sweet_w        = 34;
    wood_sweet_x        = WOOD_TRACK_X0 + 14;
    wood_success        = false;
    wood_prev_bar_x     = -1;
    ore_phase           = ORE_READY;
    ore_last_phase      = (OrePhase)0xFF;
    ore_timer           = 0;
    ore_hits            = 0;
    ore_quality         = 0;
    ore_cursor_x        = ORE_ARC_X0 + 2;
    ore_cursor_dir      = 1;
    ore_spot_x[0]       = ORE_ARC_X0 + 8;
    ore_spot_w[0]       = 16;
    ore_spot_done[0]    = false;
    ore_spot_x[1]       = ORE_ARC_X0 + 40;
    ore_spot_w[1]       = 14;
    ore_spot_done[1]    = false;
    ore_spot_x[2]       = ORE_ARC_X0 + 72;
    ore_spot_w[2]       = 12;
    ore_spot_done[2]    = false;
    ore_success         = false;
    ore_prev_cursor_x   = -1;
    mg_rng ^= (u32)(gather_type * 0x9E37u + 0x79B9u);

    /* Switch display to Mode 3 bitmap, BG2 only (hide OBJ layer). */
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    /* Draw once and keep stable while the mini-game is active. */
    draw_backdrop();
}

void minigame_update(void)
{
    /* Re-enforce Mode 3 without OBJ in case main.c's DISPCNT guard ran
       before we got here.  This is a no-op when already in Mode 3. */
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    switch (mg_phase) {

    /* ── Mini-game active ───────────────────────────────────────────────── */
    case MG_ACTIVE:
        if (active_update()) {
            REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
            mg_phase = MG_DONE;
        }
        break;

    case MG_DONE:
    case MG_INACTIVE:
    default:
        break;
    }
}

bool minigame_is_active(void)
{
    return (mg_phase != MG_INACTIVE && mg_phase != MG_DONE);
}

bool minigame_is_done(void)
{
    return (mg_phase == MG_DONE);
}

u8 minigame_get_yield(void)
{
    if (mg_score == 0) {
        return 0;
    }

    /* Base yield per resource type */
    u8 base = (mg_type == GATHER_ORE) ? 5 : 10;

    /* Bonus table indexed by score 0–4 */
    static const s8 bonus[5] = { 0, 0, 2, 5, 10 };
    u8 sc = (mg_score < 5) ? mg_score : 4;

    int result = (int)base + (int)bonus[sc];
    return (u8)(result > 255 ? 255 : result);
}

void minigame_reset(void)
{
    mg_phase = MG_INACTIVE;
    mg_score = 0;
    mg_timer = 0;
}
