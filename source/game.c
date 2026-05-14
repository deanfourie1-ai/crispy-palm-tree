/*
 * game.c  –  Hearthwood Hollow: single-hero collecting game logic.
 *
 * Willow is the hero.  The player moves her around the map with the D-pad.
 * Pressing A near a resource tile (forest / water / ore) starts the matching
 * mini-game.  After DAY_GOAL_TOTAL gathers the day ends automatically.
 *
 * HUD (drawn via OAM sprites in Mode 0):
 *   Row 0:  [LOG icon] x## [ORE icon] x## [FISH icon] x##  DAY #
 *   Bottom: ♥ ♥ ♥ ♡    minimap bottom-right
 */

#include "game.h"
#include "input.h"
#include "map.h"
#include "unit.h"
#include "tiles.h"
#include "minigame.h"
#include "video.h"

/* ── State ──────────────────────────────────────────────────────────────── */
s16 cam_x    = 0;
s16 cam_y    = 0;
s16 cursor_tx = 8;
s16 cursor_ty = 16;

int wood          = 0;
int food          = 0;
int gold          = 0;
int wood_today    = 0;
int gold_today    = 0;
int fish_today    = 0;
int game_day      = 1;
int hero_hearts   = 3;
int hovel_level   = 0;
int day_gathers_left = DAY_GOAL_TOTAL;

static bool  day_over_flag   = false;
static int   move_cooldown   = 0;   /* frames between hero steps */
#define HERO_MOVE_DELAY 8

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static bool tile_is_gatherable(u8 tile)
{
    return (tile == TILE_FOREST || tile == TILE_WATER || tile == TILE_ORE);
}

/* Find a gatherable tile adjacent to (tx,ty) or the tile itself. */
static bool find_adjacent_resource(s16 tx, s16 ty, u8 *out_tile,
                                   s16 *out_rtx, s16 *out_rty)
{
    static const s16 dx[5] = { 0, 0, 1, 0,-1};
    static const s16 dy[5] = { 0,-1, 0, 1, 0};
    for (int k = 0; k < 5; k++) {
        s16 ax = (s16)(tx + dx[k]);
        s16 ay = (s16)(ty + dy[k]);
        if (ax < 0 || ax >= MAP_W || ay < 0 || ay >= MAP_H) continue;
        u8 t = map_get_tile(ax, ay);
        if (tile_is_gatherable(t)) {
            *out_tile = t;
            *out_rtx  = ax;
            *out_rty  = ay;
            return true;
        }
    }
    return false;
}

static void apply_minigame_yield(u8 tile)
{
    u8 yield = minigame_get_yield();
    if (yield == 0) return;

    if (tile == TILE_FOREST) {
        wood += yield;
        wood_today += yield;
    } else if (tile == TILE_WATER) {
        food += yield;
        fish_today += yield;
    } else if (tile == TILE_ORE) {
        gold += yield;
        gold_today += yield;
    }

    if (day_gathers_left > 0) day_gathers_left--;
    if (day_gathers_left <= 0) day_over_flag = true;
}

/* ── OAM helpers ─────────────────────────────────────────────────────────── */
static void oam_hide(int slot)
{
    OAM_MEM[slot].attr0 = OBJ_HIDE;
    OAM_MEM[slot].attr1 = 0;
    OAM_MEM[slot].attr2 = 0;
}

/*
 * OAM layout for Hearthwood Hollow HUD:
 *   Slot  0  – Hero sprite (peasant, 8×8, follows hero pos)
 *   Slots 1-3 – Wood icon + two digit tiles
 *   Slots 4-6 – Gold icon + two digit tiles
 *   Slots 7-9 – Fish icon + two digit tiles
 *   Slot  10 – DAY digit (tens)
 *   Slot  11 – DAY digit (ones)
 *   Slots 12-15 – Heart icons (4 slots)
 */
#define OAM_HERO       0
#define OAM_WOOD_ICON  1
#define OAM_WOOD_D0    2
#define OAM_WOOD_D1    3
#define OAM_GOLD_ICON  4
#define OAM_GOLD_D0    5
#define OAM_GOLD_D1    6
#define OAM_FISH_ICON  7
#define OAM_FISH_D0    8
#define OAM_FISH_D1    9
#define OAM_DAY_D0    10
#define OAM_DAY_D1    11
#define OAM_HEART_0   12
#define OAM_HOVEL     16   /* 16×16 hovel building sprite */

/* ── Active minigame tracking ─────────────────────────────────────────────── */
static bool minigame_pending   = false;
static u8   minigame_res_tile  = TILE_GRASS;
static s16  minigame_res_tx    = 0;
static s16  minigame_res_ty    = 0;

/* ── Init ──────────────────────────────────────────────────────────────────  */
void game_init(void)
{
    cam_x             = 0;
    cam_y             = 32;
    cursor_tx         = 8;
    cursor_ty         = 16;
    wood_today        = 0;
    gold_today        = 0;
    fish_today        = 0;
    day_gathers_left  = DAY_GOAL_TOTAL;
    day_over_flag     = false;
    move_cooldown     = 0;
    minigame_pending  = false;
    /* hero stats persist across days except today totals */
}

bool game_day_over(void)
{
    return day_over_flag;
}

/* ── Satchel / inventory overlay ──────────────────────────────────────────  */

#define INV_PIX(x,y)  ((volatile u16 *)0x06000000)[(y)*240+(x)]

static void inv_fill(int x, int y, int w, int h, u16 c) {
    for (int j = y; j < y+h && j < 160; j++)
        for (int i = x; i < x+w && i < 240; i++)
            INV_PIX(i, j) = c;
}
static void inv_hline(int x, int y, int w, u16 c) {
    for (int i = x; i < x+w && i < 240; i++) if (y >= 0 && y < 160) INV_PIX(i, y) = c;
}
static void inv_vline(int x, int y, int h, u16 c) {
    for (int j = y; j < y+h && j < 160; j++) if (x >= 0 && x < 240) INV_PIX(x, j) = c;
}

static const struct { char c; u8 r[7]; } inv_fnt[] = {
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
    {':',{0x00,0x04,0x00,0x00,0x00,0x04,0x00}},
    {'/',{0x01,0x01,0x02,0x04,0x08,0x10,0x10}},
    {'+',{0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},
    {'!',{0x04,0x04,0x04,0x04,0x04,0x00,0x04}},
};
#define INV_FNTCOUNT ((int)(sizeof(inv_fnt)/sizeof(inv_fnt[0])))

static void inv_char(int x, int y, char ch, u16 fg, u16 bg) {
    for (int i = 0; i < INV_FNTCOUNT; i++) {
        if (inv_fnt[i].c != ch) continue;
        for (int row = 0; row < 7; row++) {
            u8 bits = inv_fnt[i].r[row];
            for (int col = 0; col < 5; col++) {
                int px = x+col, py = y+row;
                if (px >= 0 && px < 240 && py >= 0 && py < 160)
                    INV_PIX(px, py) = (bits & (0x10 >> col)) ? fg : bg;
            }
        }
        return;
    }
}

static void inv_str(int x, int y, const char *s, u16 fg, u16 bg) {
    for (; *s; s++, x += 6) inv_char(x, y, *s, fg, bg);
}

static int inv_strw(const char *s) {
    int n = 0; while (s[n]) n++;
    return n > 0 ? n * 6 - 1 : 0;
}

static void inv_str_c(int cx, int y, const char *s, u16 fg, u16 bg) {
    inv_str(cx - inv_strw(s)/2, y, s, fg, bg);
}

/* Simple itoa, returns pointer to buf */
static const char *inv_itoa(int n, char *buf, int blen) {
    if (blen < 2) { buf[0]='\0'; return buf; }
    if (n <= 0)   { buf[0]='0'; buf[1]='\0'; return buf; }
    int i = 0;
    while (n > 0 && i < blen-1) { buf[i++] = (char)('0'+n%10); n /= 10; }
    buf[i] = '\0';
    for (int a=0, b=i-1; a<b; a++, b--) { char t=buf[a]; buf[a]=buf[b]; buf[b]=t; }
    return buf;
}

/* Hovel stage names */
static const char *hovel_name(int lvl) {
    switch (lvl) {
        case 0: return "TENT";
        case 1: return "CABIN";
        case 2: return "COTTAGE";
        case 3: return "LONGHOUSE";
        default: return "MANOR";
    }
}

#define INV_C_BG    GBA_RGB( 6,  4,  1)
#define INV_C_PANEL GBA_RGB(18, 12,  4)
#define INV_C_GOLD  GBA_RGB(30, 24,  6)
#define INV_C_CREAM GBA_RGB(30, 28, 22)
#define INV_C_DIM   GBA_RGB(18, 15, 10)
#define INV_C_GREEN GBA_RGB( 7, 20,  5)
#define INV_C_RED   GBA_RGB(25,  7,  4)

static void draw_inv_panel(void) {
    /* Background */
    inv_fill(0, 0, 240, 160, INV_C_BG);

    /* Panel border */
    int px=20, py=16, pw=200, ph=128;
    inv_fill(px+2, py+2, pw, ph, GBA_RGB(2,1,0));  /* shadow */
    inv_fill(px, py, pw, ph, INV_C_GOLD);
    inv_fill(px+2, py+2, pw-4, ph-4, INV_C_PANEL);
    inv_hline(px+2, py+2, pw-4, GBA_RGB(25,20,8)); /* top bevel */
    inv_vline(px+2, py+2, ph-4, GBA_RGB(25,20,8)); /* left bevel */

    /* Title */
    inv_fill(px+2, py+2, pw-4, 14, GBA_RGB(12,8,2));
    inv_hline(px+2, py+16, pw-4, INV_C_GOLD);
    inv_str_c(120, py+4, "SATCHEL", INV_C_GOLD, GBA_RGB(12,8,2));

    /* Divider labels */
    int lx = px+8, ry = py+24;
    char nbuf[8];

    /* Resources */
    inv_str(lx, ry+0,  "OAK LOGS :", INV_C_DIM, INV_C_PANEL);
    inv_str(lx+80, ry+0, inv_itoa(wood, nbuf, 8), INV_C_CREAM, INV_C_PANEL);

    inv_str(lx, ry+14, "GOLD ORE :", INV_C_DIM, INV_C_PANEL);
    inv_str(lx+80, ry+14, inv_itoa(gold, nbuf, 8), INV_C_GOLD, INV_C_PANEL);

    inv_str(lx, ry+28, "RIVER FISH:", INV_C_DIM, INV_C_PANEL);
    inv_str(lx+80, ry+28, inv_itoa(food, nbuf, 8), INV_C_GREEN, INV_C_PANEL);

    /* Separator */
    inv_hline(px+6, ry+40, pw-12, GBA_RGB(10,7,2));

    /* Hero status */
    inv_str(lx, ry+48, "HEARTS  :", INV_C_DIM, INV_C_PANEL);
    {
        char hbuf[8];
        int bi=0;
        hbuf[bi++] = (char)('0'+hero_hearts);
        hbuf[bi++] = '/';
        hbuf[bi++] = (char)('0'+HERO_MAX_HEARTS);
        hbuf[bi]   = '\0';
        inv_str(lx+80, ry+48, hbuf, INV_C_RED, INV_C_PANEL);
    }

    inv_str(lx, ry+62, "DAY     :", INV_C_DIM, INV_C_PANEL);
    inv_str(lx+80, ry+62, inv_itoa(game_day, nbuf, 8), INV_C_CREAM, INV_C_PANEL);

    inv_str(lx, ry+76, "HOVEL   :", INV_C_DIM, INV_C_PANEL);
    inv_str(lx+80, ry+76, hovel_name(hovel_level), INV_C_GOLD, INV_C_PANEL);

    /* Close hint */
    inv_str_c(120, py+ph-12, "B / SELECT  TO CLOSE", INV_C_DIM, INV_C_PANEL);
}

/* ── Per-frame update ──────────────────────────────────────────────────────  */
void game_update(void)
{
    /* ── Mini-game overlay ─────────────────────────────────────────────── */
    if (minigame_is_active()) {
        minigame_update();
        if (minigame_is_done()) {
            /* Restore tile-mode display */
            video_init();
            map_init();
            /* Apply yield */
            apply_minigame_yield(minigame_res_tile);
            minigame_reset();
            minigame_pending = false;
        }
        return;
    }

    /* ── Hero movement (D-pad, tile-by-tile with cooldown) ─────────────── */
    if (move_cooldown > 0) {
        move_cooldown--;
    } else {
        s16 ntx = cursor_tx, nty = cursor_ty;
        bool moved = false;

        if (input_held(KEY_RIGHT) && cursor_tx < MAP_W - 1) { ntx++; moved = true; }
        else if (input_held(KEY_LEFT)  && cursor_tx > 0)         { ntx--; moved = true; }
        else if (input_held(KEY_DOWN)  && cursor_ty < MAP_H - 1) { nty++; moved = true; }
        else if (input_held(KEY_UP)    && cursor_ty > 0)         { nty--; moved = true; }

        if (moved) {
            /* Don't walk into blocking tiles (mountains, impassable water) */
            u8 next_tile = map_get_tile(ntx, nty);
            bool blocks = (next_tile == TILE_MOUNTAIN);
            if (!blocks) {
                cursor_tx = ntx;
                cursor_ty = nty;
                move_cooldown = HERO_MOVE_DELAY;
            }
        }
    }

    /* ── A button: start gather mini-game ──────────────────────────────── */
    if (input_pressed(KEY_A) && !minigame_pending && day_gathers_left > 0) {
        u8 res_tile;
        s16 rtx, rty;
        if (find_adjacent_resource(cursor_tx, cursor_ty, &res_tile, &rtx, &rty)) {
            minigame_res_tile = res_tile;
            minigame_res_tx   = rtx;
            minigame_res_ty   = rty;
            minigame_pending  = true;
            u8 gather_type = (res_tile == TILE_FOREST) ? GATHER_WOOD :
                             (res_tile == TILE_WATER)  ? GATHER_FISH :
                                                         GATHER_ORE;
            minigame_start(gather_type);
        }
    }

    /* ── Start button: end day early ────────────────────────────────────── */
    if (input_pressed(KEY_START)) {
        day_over_flag = true;
    }

    /* ── Select button: open satchel inventory overlay ──────────────────── */
    if (input_pressed(KEY_SELECT) && !minigame_is_active()) {
        REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
        draw_inv_panel();
        u16 prev_k = REG_KEYINPUT;
        while (true) {
            vblank_wait();
            u16 cur_k = REG_KEYINPUT;
            bool close = (((prev_k & KEY_B)      != 0) && ((cur_k & KEY_B)      == 0)) ||
                         (((prev_k & KEY_SELECT)  != 0) && ((cur_k & KEY_SELECT) == 0));
            prev_k = cur_k;
            if (close) break;
        }
        video_init();
        map_init();
    }

    /* ── Camera follows hero (pixel-smooth, with margin) ───────────────── */
    int cx_px = cursor_tx * 8;
    int cy_px = cursor_ty * 8;
#define CAM_MARGIN 24
    if (cx_px - cam_x > SCREEN_W - CAM_MARGIN) cam_x = (s16)(cx_px - (SCREEN_W - CAM_MARGIN));
    if (cx_px - cam_x < CAM_MARGIN)             cam_x = (s16)(cx_px - CAM_MARGIN);
    if (cy_px - cam_y > SCREEN_H - CAM_MARGIN)  cam_y = (s16)(cy_px - (SCREEN_H - CAM_MARGIN));
    if (cy_px - cam_y < CAM_MARGIN)             cam_y = (s16)(cy_px - CAM_MARGIN);
    if (cam_x < 0)         cam_x = 0;
    if (cam_x > CAM_MAX_X) cam_x = (s16)CAM_MAX_X;
    if (cam_y < 0)         cam_y = 0;
    if (cam_y > CAM_MAX_Y) cam_y = (s16)CAM_MAX_Y;

    map_set_scroll(cam_x, cam_y);
}

/* ── HUD ────────────────────────────────────────────────────────────────── */
void game_hud_draw(void)
{
    volatile OBJ_ATTR *oam = OAM_MEM;

    /* ── Hero sprite at tile position (16×16 Willow) ───────────────────── */
    {
        /* Centre the 16×16 sprite horizontally on the 8×8 world tile and
         * place the feet near the tile bottom (−12 shifts the top of the
         * sprite 4 px above the tile top, feet land at tile_y + 4).      */
        int sx = cursor_tx * 8 - cam_x - 4;
        int sy = cursor_ty * 8 - cam_y - 12;
        if (sx >= -16 && sx < SCREEN_W && sy >= -16 && sy < SCREEN_H) {
            oam[OAM_HERO].attr0 = OBJ_Y(sy) | OBJ_SHAPE_SQ | OBJ_4BPP;
            oam[OAM_HERO].attr1 = OBJ_X(sx) | OBJ_SIZE_16;
            oam[OAM_HERO].attr2 = OBJ_TILE(OBJ_TILE_WILLOW_BASE) | OBJ_PRIO(1) | OBJ_PAL(0);
        } else {
            oam_hide(OAM_HERO);
        }
    }

    /* ── Resource counter row (top HUD bar) ────────────────────────────── */
    int w_clamped = wood > 99 ? 99 : wood;
    int g_clamped = gold > 99 ? 99 : gold;
    int f_clamped = food > 99 ? 99 : food;

    /* Wood icon + two digits at x = 4, 12, 20 */
    oam[OAM_WOOD_ICON].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_WOOD_ICON].attr1 = OBJ_X(4) | OBJ_SIZE_8;
    oam[OAM_WOOD_ICON].attr2 = OBJ_TILE(OBJ_TILE_WOOD_ICON) | OBJ_PRIO(0) | OBJ_PAL(0);
    oam[OAM_WOOD_D0].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_WOOD_D0].attr1 = OBJ_X(12) | OBJ_SIZE_8;
    oam[OAM_WOOD_D0].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + w_clamped/10) | OBJ_PRIO(0) | OBJ_PAL(0);
    oam[OAM_WOOD_D1].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_WOOD_D1].attr1 = OBJ_X(20) | OBJ_SIZE_8;
    oam[OAM_WOOD_D1].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + w_clamped%10) | OBJ_PRIO(0) | OBJ_PAL(0);

    /* Gold icon at x = 36 */
    oam[OAM_GOLD_ICON].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_GOLD_ICON].attr1 = OBJ_X(36) | OBJ_SIZE_8;
    oam[OAM_GOLD_ICON].attr2 = OBJ_TILE(OBJ_TILE_ORE_ICON) | OBJ_PRIO(0) | OBJ_PAL(0);
    oam[OAM_GOLD_D0].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_GOLD_D0].attr1 = OBJ_X(44) | OBJ_SIZE_8;
    oam[OAM_GOLD_D0].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + g_clamped/10) | OBJ_PRIO(0) | OBJ_PAL(0);
    oam[OAM_GOLD_D1].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_GOLD_D1].attr1 = OBJ_X(52) | OBJ_SIZE_8;
    oam[OAM_GOLD_D1].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + g_clamped%10) | OBJ_PRIO(0) | OBJ_PAL(0);

    /* Fish icon at x = 68 */
    oam[OAM_FISH_ICON].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_FISH_ICON].attr1 = OBJ_X(68) | OBJ_SIZE_8;
    oam[OAM_FISH_ICON].attr2 = OBJ_TILE(OBJ_TILE_FISH_ICON) | OBJ_PRIO(0) | OBJ_PAL(0);
    oam[OAM_FISH_D0].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_FISH_D0].attr1 = OBJ_X(76) | OBJ_SIZE_8;
    oam[OAM_FISH_D0].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + f_clamped/10) | OBJ_PRIO(0) | OBJ_PAL(0);
    oam[OAM_FISH_D1].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_FISH_D1].attr1 = OBJ_X(84) | OBJ_SIZE_8;
    oam[OAM_FISH_D1].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + f_clamped%10) | OBJ_PRIO(0) | OBJ_PAL(0);

    /* Day counter top-right (two digits) */
    int d = game_day > 99 ? 99 : game_day;
    oam[OAM_DAY_D0].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_DAY_D0].attr1 = OBJ_X(208) | OBJ_SIZE_8;
    oam[OAM_DAY_D0].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + d/10) | OBJ_PRIO(0) | OBJ_PAL(0);
    oam[OAM_DAY_D1].attr0 = OBJ_Y(2) | OBJ_SHAPE_SQ | OBJ_4BPP;
    oam[OAM_DAY_D1].attr1 = OBJ_X(216) | OBJ_SIZE_8;
    oam[OAM_DAY_D1].attr2 = OBJ_TILE(OBJ_TILE_DIGIT_0 + d%10) | OBJ_PRIO(0) | OBJ_PAL(0);

    /* Heart row bottom-left (4 hearts) */
    for (int i = 0; i < HERO_MAX_HEARTS; i++) {
        int hx = 4 + i * 10;
        int hy = 148;
        oam[OAM_HEART_0 + i].attr0 = OBJ_Y(hy) | OBJ_SHAPE_SQ | OBJ_4BPP;
        oam[OAM_HEART_0 + i].attr1 = OBJ_X(hx) | OBJ_SIZE_8;
        u8 htile = (i < hero_hearts) ? OBJ_TILE_HEART_FULL : OBJ_TILE_HEART_EMPTY;
        oam[OAM_HEART_0 + i].attr2 = OBJ_TILE(htile) | OBJ_PRIO(0) | OBJ_PAL(0);
    }

    /* ── Hovel building sprite (16×16) at fixed world position ────────────── */
    /* Place the hovel at tile (26, 16) — centre the sprite on that tile */
#define HOVEL_TX  26
#define HOVEL_TY  16
    {
        int hx = HOVEL_TX * 8 - cam_x - 4;
        int hy = HOVEL_TY * 8 - cam_y - 8;
        if (hx >= -16 && hx < SCREEN_W && hy >= -16 && hy < SCREEN_H) {
            oam[OAM_HOVEL].attr0 = OBJ_Y(hy) | OBJ_SHAPE_SQ | OBJ_4BPP;
            oam[OAM_HOVEL].attr1 = OBJ_X(hx) | OBJ_SIZE_16;
            oam[OAM_HOVEL].attr2 = OBJ_TILE(OBJ_TILE_HOVEL_BASE) | OBJ_PRIO(2) | OBJ_PAL(0);
        } else {
            oam_hide(OAM_HOVEL);
        }
    }

    /* Hide unused OAM slots (17 and above) */
    for (int i = OAM_HOVEL + 1; i < 128; i++)
        oam_hide(i);
}
