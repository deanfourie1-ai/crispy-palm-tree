# Hearthwood Hollow — Developer Context

> **Read this file at the start of every new session.**
> It is the single source of truth for architecture, file layout, palette,
> OAM slots, known gotchas and planned next steps.

---

## 1. What this game is

**Hearthwood Hollow** is a single-hero GBA collecting game.
Willow (the hero) walks around a scrollable overworld, gathers Wood / Gold / Fish,
and completes a mini-game for each resource. After 6 gathers the day ends and a
results screen is shown. The player can upgrade their hovel between days and
view a world map. After enough days the game loops back to the title.

ROM output: `hearthwood.gba` (≈36 KB as of commit `b260616`).

---

## 2. Toolchain & build

**Host OS**: Windows 11. Build runs inside WSL Ubuntu.

### Build command (from Windows PowerShell / terminal)
```powershell
wsl -d Ubuntu -- bash -c "cd '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree' && make 2>&1"
```

### Clean build
```powershell
wsl -d Ubuntu -- bash -c "cd '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree' && make clean && make 2>&1"
```

> **Warning**: `make clean` deletes `hearthwood.gba`. If the emulator has the ROM
> open, the delete will fail with "Permission denied". Close the emulator first,
> or skip `make clean` and rely on incremental builds.

### Write final ROM after build (if objcopy was blocked)
```powershell
wsl -d Ubuntu -- bash -c "arm-none-eabi-objcopy -O binary '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree/build/hearthwood.elf' '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree/hearthwood.gba'"
```

### Toolchain packages
```bash
sudo apt install -y build-essential make gcc-arm-none-eabi binutils-arm-none-eabi
```

---

## 3. Repository layout

```
crispy-palm-tree/
├── Makefile                  arm-none-eabi-gcc, outputs hearthwood.gba
├── gba_cart.ld               GBA cartridge linker script
├── include/
│   ├── gba.h                 Hardware registers, types (u8/u16/s16/bool), GBA_RGB macro
│   ├── tiles.h               ALL tile pixel data + BG/OBJ palettes  ← most edits go here
│   ├── video.h / video.c     Mode 0 init (BG0 world + BG1 HUD strip)
│   ├── input.h / input.c     KEY_* bitmasks, input_pressed(), input_held()
│   ├── map.h / map.c         Tile map, TILE_* constants, map_get_tile(), map_set_tile()
│   ├── unit.h / unit.c       Peasant/unit structs (legacy, still compiled)
│   ├── game.h / game.c       Hero movement, HUD draw, satchel inventory, day logic
│   ├── menu.h / menu.c       Title screen (Mode 3)
│   ├── minigame.h / minigame.c  Three mini-games (Mode 3)
│   └── results.h / results.c   Results screen, hovel upgrade, world map (Mode 3)
├── source/
│   ├── crt0.s                ARM startup + GBA ROM header (game title = "HEARTHWOOD")
│   ├── runtime.c             Freestanding memcpy/memset/strlen shims
│   └── main.c                Entry point and phase loop
├── build/                    Generated (objects, deps, ELF)
├── hearthwood.gba            Final playable ROM
├── DEV_LOG.md                Chronological change log
├── CONTEXT.md                ← this file
└── compile instructions.md  Quick build reference
```

---

## 4. Game phase flow

```
main()
 │
 ├─► TITLE  (menu.c, Mode 3)
 │     Press START → reset wood/food/gold/game_day/hero_hearts/hovel_level/streak
 │
 └─► DAY LOOP
       │
       ├─► GAME  (game.c, Mode 0 + minigame.c Mode 3 overlays)
       │     D-pad moves Willow   A gathers nearest resource
       │     Start ends day early SELECT opens satchel inventory
       │     day_gathers_left counts down from 6 → 0 → day over
       │
       └─► RESULTS  (results.c, Mode 3)
             A  → advance to next day (game_day++, today totals reset)
             B  → upgrade hovel (if affordable)
             SELECT → view world map (press any key to return)
             hero_hearts <= 0 → back to title
```

---

## 5. GBA video modes used

| Mode | DISPCNT value | Used for |
|------|--------------|----------|
| Mode 0 | `DCNT_MODE0 \| DCNT_BG0 \| DCNT_BG1 \| DCNT_OBJ \| DCNT_OBJ_1D` | Overworld + HUD |
| Mode 3 | `DCNT_MODE3 \| DCNT_BG2` | Title / minigames / results / satchel |

When returning from Mode 3 back to Mode 0, always restore:
```c
REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
```
Also call `map_init()` to re-upload the tilemap to SBB 24/25.

---

## 6. BG layout (Mode 0)

| Layer | SBB | CBB | Purpose |
|-------|-----|-----|---------|
| BG0 | 24 + 25 | 0 | Scrolling world map (64×32 tiles) |
| BG1 | 26 | 0 | Fixed HUD strip (row 0 only) |

- BG palette colour 0 = `GBA_RGB(0,0,0)` = **transparent** for both layers.
- BG tile index 6 = all-zero bytes → transparent (used to clear BG1 rows 1+).
- BG tile index 7 = HUD strip (6 rows warm-brown / 1 parchment / 1 gold edge).
- `REG_BG1HOFS = REG_BG1VOFS = 0` — BG1 never scrolls.

---

## 7. OBJ (sprite) system

### 1D tile mapping
- `DCNT_OBJ_1D` is set → tiles are laid out linearly in VRAM.
- A 16×16 OBJ uses 4 consecutive tiles: TL, TR, BL, BR (row-major).
- An 8×8 OBJ uses 1 tile.

### 4-bpp encoding (in `tiles.h`)
Each byte stores two pixels: `byte = pixel[even_col] | (pixel[odd_col] << 4)`
- Low nibble = left (even) pixel.
- High nibble = right (odd) pixel.
- Palette index 0 = transparent.

### OAM slot map
| Slot | Symbol | Size | Content |
|------|--------|------|---------|
| 0 | `OAM_HERO` | 16×16 | Willow hero sprite (tiles 49-52) |
| 1 | `OAM_WOOD_ICON` | 8×8 | Wood icon (tile 5) |
| 2-3 | `OAM_WOOD_D0/D1` | 8×8 | Wood count digits |
| 4 | `OAM_GOLD_ICON` | 8×8 | Gold icon (tile 7) |
| 5-6 | `OAM_GOLD_D0/D1` | 8×8 | Gold count digits |
| 7 | `OAM_FISH_ICON` | 8×8 | Fish icon (tile 6) |
| 8-9 | `OAM_FISH_D0/D1` | 8×8 | Fish count digits |
| 10-11 | `OAM_DAY_D0/D1` | 8×8 | Day number digits |
| 12-15 | `OAM_HEART_0..3` | 8×8 | Heart icons (full=53 / empty=54) |
| 16 | `OAM_HOVEL` | 16×16 | Hovel building (tiles 55-58) |
| 17-127 | — | — | Hidden every frame |

---

## 8. Tile index map (`obj_tiles[]` in tiles.h)

```
[0]       OBJ_TILE_FOOTMAN         8×8  (legacy)
[1]       OBJ_TILE_ORC             8×8  (legacy)
[2]       OBJ_TILE_CURSOR          8×8
[3]       OBJ_TILE_PEASANT         8×8
[4]       OBJ_TILE_HIGHLIGHT       8×8
[5]       OBJ_TILE_WOOD_ICON       8×8  HUD wood log
[6]       OBJ_TILE_FISH_ICON       8×8  HUD fish
[7]       OBJ_TILE_ORE_ICON        8×8  HUD gold diamond
[8-11]    OBJ_TILE_WOOD_STORE      16×16
[12-15]   OBJ_TILE_FOOD_STORE      16×16
[16-19]   OBJ_TILE_GOLD_STORE      16×16
[20-29]   OBJ_TILE_DIGIT_0 (+0-9)  8×8  digits 0-9
[30]      OBJ_TILE_UI_FRAME_CORNER 8×8
[31]      OBJ_TILE_UI_FRAME_EDGE   8×8
[32]      OBJ_TILE_UI_FRAME_FILL   8×8
[33-37]   OBJ_TILE_LETTER_B/U/I/L/D 8×8
[38-41]   OBJ_TILE_BUILD_FOUNDATION 16×16
[42-45]   OBJ_TILE_BUILD_PROGRESS   16×16
[46]      OBJ_TILE_PEASANT_LIVELY   8×8
[47]      OBJ_TILE_PEASANT_WORK     8×8
[48]      OBJ_TILE_PEASANT_FISH     8×8
[49-52]   OBJ_TILE_WILLOW_BASE      16×16  Willow hero (TL/TR/BL/BR)
[53]      OBJ_TILE_HEART_FULL       8×8   gold filled ♥
[54]      OBJ_TILE_HEART_EMPTY      8×8   dark-red hollow ♡
[55-58]   OBJ_TILE_HOVEL_BASE       16×16  cabin building (TL/TR/BL/BR)
```

Total: `obj_tiles[59][32]`

---

## 9. Palette

### BG palette (bg_palette[16])
```
 0  transparent / black
 1  grass light    #688a3a  GBA_RGB(13,17, 7)
 2  grass mid      #446a26  GBA_RGB( 8,13, 5)
 3  forest dark    #244a18  GBA_RGB( 4, 9, 3)
 4  water light    #3a5e9e  GBA_RGB( 7,11,19)
 5  water deep     #1e3a72  GBA_RGB( 3, 7,14)
 6  stone light    #6a6878  GBA_RGB(13,13,15)
 7  stone dark     #3a3848  GBA_RGB( 7, 7, 9)
 8  earth/path     #b08858  GBA_RGB(22,17,11)
 9  earth light    #d4ac80  GBA_RGB(26,21,16)
10  white                   GBA_RGB(31,31,31)
11  black                   GBA_RGB( 0, 0, 0)
12  gold ore vein  #f0c030  GBA_RGB(30,24, 6)
13-15  unused (black)
```

### OBJ palette (obj_palette[16])
```
 0  transparent
 1  skin           #FFCC99  GBA_RGB(31,25,19)
 2  water blue     #3078CC  GBA_RGB( 6,15,25)
 3  water deep     #184080  GBA_RGB( 3, 8,16)
 4  leaf green     #509028  GBA_RGB(10,18, 5)
 5  leaf dark      #205010  GBA_RGB( 4,10, 2)
 6  red            #CC2222  GBA_RGB(25, 4, 4)   ← hovel roof
 7  dark red       #880000  GBA_RGB(17, 0, 0)   ← hovel roof shadow / empty heart
 8  warm gold      #f0c030  GBA_RGB(30,24, 6)   ← full heart / window glass / hat brim
 9  gold shadow    #c09010  GBA_RGB(24,18, 2)   ← window frame / hat crown
10  white                   GBA_RGB(31,31,31)
11  black                   GBA_RGB( 0, 0, 0)
12  willow tunic   #884820  GBA_RGB(17, 9, 4)   ← walls / tunic
13  tunic shadow   #603010  GBA_RGB(12, 6, 2)   ← dark walls / door frame
14  hair + boots   #3a1808  GBA_RGB( 7, 3, 1)
15  pants          #3a2818  GBA_RGB( 7, 5, 3)
```

---

## 10. Key globals (game.c / game.h)

```c
s16 cam_x, cam_y;          // camera top-left pixel
s16 cursor_tx, cursor_ty;  // hero tile position
int wood, food, gold;      // lifetime totals
int wood_today, gold_today, fish_today;  // reset each day
int game_day;              // 1-based
int hero_hearts;           // 0-4, starts at 3
int hovel_level;           // 0-4, persists, resets on new game
int day_gathers_left;      // counts down from DAY_GOAL_TOTAL (6) → 0
```

---

## 11. Hovel upgrade costs (results.c)

```c
static const int hovel_wood_cost[4] = { 20, 30, 45, 60 };
static const int hovel_gold_cost[4] = {  0, 15, 25, 40 };
static const int hovel_fish_cost[4] = {  0,  0,  0, 20 };
```
Level names: 0=TENT, 1=CABIN, 2=COTTAGE, 3=LONGHOUSE, 4=MANOR

---

## 12. Map constants (map.h)

```c
#define MAP_W   64
#define MAP_H   32
#define MAP_CBB  0   // character base block
#define TILE_GRASS   0
#define TILE_WATER   1
#define TILE_FOREST  2
#define TILE_MOUNTAIN 3
#define TILE_ORE     5
```

Hovel world position (hard-coded in game.c): **tile (26, 16)**.

---

## 13. Mode 3 pixel font (duplicated in minigame.c, results.c, game.c)

Each file that draws Mode 3 text has an inline pixel font array (`inv_fnt[]` or similar).
Glyph dimensions: 5 wide × 7 tall. Character advance: 6 px.
Supported characters: A-Z, 0-9, `:`, `/`, `+`, `!`.
`MODE3_MEM` base = `(volatile u16*)0x06000000`; stride = 240 u16s per row.

---

## 14. Known quirks / gotchas

1. **`make clean` permission error** — Close the emulator before running `make clean`.
   Alternatively use incremental `make` or manually delete `hearthwood.gba` first.

2. **4-bpp nibble order** — Low nibble = even column, high nibble = odd column.
   `byte = pixel[even] | (pixel[odd] << 4)`.
   A common mistake is swapping them; always decode a few bytes to verify pixel layout.

3. **Mode 0 → Mode 3 transitions** — After any Mode 3 screen (minigame, results, inventory):
   - Call `video_init()` or manually set `REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D`.
   - Call `map_init()` to restore BG tilemap in VRAM (Mode 3 overwrites the same VRAM region).

4. **BG1 not visible after minigame** — Ensure `DCNT_BG1` is included in every Mode 0 DISPCNT restore.
   It was missing in two places and caused the HUD strip to disappear.

5. **1D OBJ tile mapping** — A 16×16 OBJ with base tile N uses tiles N (TL), N+1 (TR), N+2 (BL), N+3 (BR).
   Make sure tile indices are consecutive and the array is large enough.

6. **`obj_tiles` size** — Currently `obj_tiles[59][32]`. Adding new tiles: increment the array bound AND
   add the new `#define OBJ_TILE_*` constant. `video.c` uploads `sizeof(obj_tiles)` bytes.

7. **`gh` CLI not installed** — GitHub CLI is not available on this machine. PRs must be created via
   the GitHub web UI at `https://github.com/deanfourie1-ai/crispy-palm-tree`.

---

## 15. Suggested next steps

These are ideas in rough priority order. None are started yet.

### Gameplay
- [ ] **Willow walk animation** — Alternate between two 16×16 frames (add tiles 59-66, toggle on step).
- [ ] **Resource respawn** — Forest/ore/water tiles respawn after N days so the map never empties.
- [ ] **Hovel visual on map** — Swap the cabin sprite for progressively larger buildings as `hovel_level` increases (tiles 55-58 already handle level 0; need variants for levels 1-4).
- [ ] **Day night cycle** — Fade palette toward blue at high `game_day` count to indicate time passing.
- [ ] **Villager NPC** — A roaming 8×8 NPC that gives a one-line hint when Willow stands adjacent.

### UI / Polish
- [ ] **Minigame score feedback** — Show "GREAT!" / "GOOD" / "MISS" overlay in Mode 3 after each mini-game result.
- [ ] **Satchel item icons** — Add small 8×8 icons for Wood/Gold/Fish inside the satchel panel instead of text only.
- [ ] **Results screen animated totals** — Count up the numbers frame-by-frame like a classic RPG.
- [ ] **Title screen animation** — Scroll or fade the title card on entry.

### Technical
- [ ] **Sound** — GBA Direct Sound: short SFX for gather, step, minigame success/fail.
- [ ] **Save/load** — Use SRAM at `0x0E000000` (32 KB) to persist `wood/food/gold/game_day/hovel_level`.
- [ ] **Multiple maps** — Add a `MapId` selector on the title screen; each map has different resource density.
- [ ] **GitHub PR workflow** — Install `gh` CLI in WSL Ubuntu (`sudo apt install gh`) for future PRs.

---

## 16. Commit history (recent)

| Hash | Date | Summary |
|------|------|---------|
| `b260616` | 2026-05-14 | Hat on Willow + hovel building OBJ sprite |
| `50ae54f` | 2026-05-14 | Steps 1-7: HUD, minigames, hearts, satchel, world map, hovel upgrades |
| earlier | 2026-05-02 | Day 3: three-resource system, fishing animation |
| earlier | 2026-05-02 | Day 2: menu freeze fix, pathfinding fix |
| earlier | 2026-04-30 | Day 1: toolchain, cursor, menu, resource buildings |

Remote: `https://github.com/deanfourie1-ai/crispy-palm-tree`  
Branch: `main`
