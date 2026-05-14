# Dev Log — Day 1

Date: 2026-04-30

Entries:

- 2026-04-30: Day 1 — Created development log per user request. Repository: crispy-palm-tree. Started tracking progress and notes here.
- 2026-04-30: Toolchain setup completed with WSL Ubuntu (`make`, `gcc-arm-none-eabi`, `binutils-arm-none-eabi`).
- 2026-04-30: Fixed build blockers (`source/unit.c` include for building constants; added freestanding `memcpy`/`memset` in `source/runtime.c` for `-nostdlib` link).
- 2026-04-30: Investigated missing cursor visibility. Improved cursor sprite contrast in `source/tiles.h` and reinforced OBJ enable in `source/main.c`. Rebuilt ROM successfully (`warcraftgba.gba`, 6028 bytes).
- 2026-04-30: Added a new startup main menu with a fantasy blue/gold visual theme and menu navigation (`New Game`, `Settings`, `Load Game`). `New Game` now transitions into the playable map.
- 2026-04-30: Rebranded ROM output and header identity from `warcraftgba` to `embercrown` (Makefile target + GBA header title/code/checksum). Updated docs and compile instructions.
- 2026-04-30: Added freestanding `strlen` shim in `source/runtime.c` to satisfy linker in `-nostdlib` builds after menu integration.
- 2026-04-30: Implemented `Settings` controls screen under main menu in `source/menu.c` with key mappings and screen-state navigation; added right/left paging between `Controls` and `Resources` sub-screens.
- 2026-04-30: Added in-game exit path: `Start` now returns from game state to main menu (`source/main.c`), and controls docs updated to reflect this.
- 2026-04-30: Added gatherable forest clusters to map and runtime tile mutation API (`map_set_tile`) in `source/map.c`; redesigned forest tile art in `source/tiles.h` for clearer tree readability.
- 2026-04-30: Added first resource HUD counter (wood icon + 3-digit value) using OBJ overlays in `source/game.c`.
- 2026-04-30: Implemented peasant gather loop state machine: chop at forest -> carry wood -> walk to wood storage -> deposit on arrival (`source/game.c`, `source/unit.h`, `source/unit.c`).
- 2026-04-30: Expanded settings documentation UI to show resource-to-building mapping (`WOOD DEPOT`, `GOLD MINE`, `FOOD STORE`) on the resources screen.
- 2026-04-30: Upgraded resource buildings from 8x8 to 16x16 sprite sets and created new stylized pixel art variants for wood depot, food store, and gold vault (`source/tiles.h`, `source/unit.c`).
- 2026-04-30: Fixed major building/counter sprite corruption caused by 16x16 OBJ tile-base alignment in 1D mapping by realigning tile indices and adding padding tiles; performed full clean rebuild to ensure updated tile table upload path (`source/video.c`) was rebuilt.
- 2026-04-30: End-of-day stable test build validated and synced to launcher target (`test_builds/embercrown_20260430_231838.gba` -> `embercrown_test.gba`).

# Dev Log — Day 2

Date: 2026-05-02

- 2026-05-02: Diagnosed and fixed post-menu game-freeze regression. Root cause: old single-loop state machine re-entered the menu branch on the first game frame because the A-button press from map selection was still held. Refactored `source/main.c` into explicit nested loops (menu phase / game phase) so transitions are immediate and unambiguous.
- 2026-05-02: Stabilised VBlank sync by switching `vblank_wait()` from `DISPSTAT` interrupt polling to `VCOUNT`-based spin-wait (`gba.h`).
- 2026-05-02: Fixed menu re-entry corruption: `menu_update()` in `source/menu.c` now resets `current_screen = SCREEN_MAIN` before returning `MENU_ACTION_NEW_GAME`, ensuring a clean state on each new-game launch.
- 2026-05-02: Removed entire BFS forest-cluster system (`build_forest_clusters`, static cluster map array, `find_nearest_forest_in_cluster`, `ensure_forest_clusters_ready`). The BFS over 2048 tiles caused a visible freeze when a peasant was assigned to auto-gather. Replaced with `find_nearest_forest_near()` — a simple Manhattan-distance radius scan (radius 16) with no static allocation and no precompute step.
- 2026-05-02: Repurposed `Unit.gather_cluster` (s16) to store packed origin tile coordinate as `(ty<<6)|tx` instead of a cluster ID. No struct changes needed; valid range fits in s16.
- 2026-05-02: Fixed unit pathfinding getting permanently stuck when blocked by other meeples (`source/unit.c`). Old code tried only one perpendicular direction and cancelled the move target if that was also blocked. New logic: tries both perpendicular directions (preferred side first, opposite second); if all three directions are blocked by solid terrain/buildings the move is cancelled, but if blocked only by other units the target is preserved and the unit waits for the blocker to move before retrying.

Next steps:

- Continue the log daily; append entries for each work session.

---

# Dev Log — Day 3

Date: 2026-05-02

- 2026-05-02: Implemented three-resource system: added `food` and `gold` globals alongside `wood`; introduced ore tile (`TILE_ORE = 5`) to map terrain with seeding in mountain regions; extended peasant gather FSM to discriminate `gather_type` (GATHER_WOOD, GATHER_FISH, GATHER_ORE) based on target tile and assign it during A-button command issuance.
- 2026-05-02: Added gather-type-aware deposit logic: peasants carrying resources route to the corresponding building (wood → wood depot, fish → food store, ore → gold vault) and deposit appropriately; ore tiles deplete to mountain after gathering (one-time resource).
- 2026-05-02: Expanded HUD from single wood counter to three-counter display (food, gold, wood) with new OBJ sprites: fish icon (tile 6) and ore/diamond icon (tile 7); implemented digit rendering for all three resources and repositioned build menu to OAM slot 25 to avoid overlap.
- 2026-05-02: Added gathering animations tied to `gather_type`: fishing shows new rod-casting pose (`SPR_PEASANT_FISH`, tile 48 — peasant with arm extended in casting motion); mining and wood gathering both use the building/work animation (`SPR_PEASANT_WORK`, tile 47 — raised-arm tool pose). Animation dispatch updated in `unit_sprite_tile()` to check `gather_type` when `peasant_is_working()` is true.
- 2026-05-02: Tested build via `wsl -d Ubuntu -- bash -lc` compile command. All systems compiling cleanly.

---

# Dev Log — Day 4  (Major Rework: Hearthwood Hollow)

Date: 2026-05-14

The project was redesigned from a multi-unit RTS into a single-hero collecting game called **Hearthwood Hollow**.
Willow is the player character. Each day she gathers Wood, Gold and Fish across the overworld map;
after 6 gathers the day ends and a results screen is shown.

### Architecture changes

- **Phases**: `PHASE_TITLE` → `PHASE_GAME` → `PHASE_RESULTS` (loop back to game each day).
- **Mode 0** (BG0 scrolling world + BG1 fixed HUD strip + OAM sprites) used for overworld.
- **Mode 3** (240×160 16-bit bitmap) used for title, minigames, results, and satchel inventory screens.
- `source/main.c`: Outer loop = title; inner loop = day loop (game → results → next day).
  Resets `wood/food/gold/game_day/hero_hearts/hovel_level/streak` on new game.
- `source/game.c`: Hero movement (D-pad, 8-frame cooldown), A-button gathers nearest resource tile,
  Start ends the day early. SELECT opens the satchel inventory (Mode 3 panel, close with B/SELECT).
- `source/minigame.c`: Three mini-games (SWING TRUE / WATCH AND WAIT / HIT THE VEIN) for forest/water/ore.
  Each is a Mode 3 timed reaction game. On exit, restores Mode 0 with BG0+BG1+OBJ.
- `source/results.c`: Full Mode 3 results screen. Shows today's haul, lifetime totals, streak,
  hovel upgrade button (B), world map (SELECT), press A to continue.
- `source/video.c`: Sets up BG0 (world, CBB=0, SBB 24/25) and BG1 (HUD strip, SBB 26).
  BG1 row 0 = warm-brown HUD tile (index 7); rest = transparent tile (index 6).
- `include/game.h`: Added `hovel_level` extern and `HOVEL_MAX_LEVEL 4`.

### Tile / palette additions (include/tiles.h)

- `bg_tiles[8]`: index 6 = all-zero transparent tile, index 7 = HUD strip (6 rows warm-brown, 1 parchment, 1 gold).
- `obj_tiles[59]`: extended from 53 to 59 entries.
  - Tile 49-52: Willow hero 16×16 (top-down idle).
  - Tile 53: full gold heart ♥.
  - Tile 54: hollow dark-red heart ♡.
  - Tiles 55-58: hovel building 16×16 (red roof, brown walls, gold window, dark-brown door).
- OBJ palette slots 14/15: `#3a1808` (hair/boots), `#3a2818` (pants).
- New defines: `OBJ_TILE_WILLOW_BASE 49`, `OBJ_TILE_HEART_FULL 53`,
  `OBJ_TILE_HEART_EMPTY 54`, `OBJ_TILE_HOVEL_BASE 55`.

### OAM layout (source/game.c)

| Slot | Content |
|------|---------|
| 0  | Willow hero 16×16 |
| 1-3 | Wood icon + 2 digits |
| 4-6 | Gold icon + 2 digits |
| 7-9 | Fish icon + 2 digits |
| 10-11 | Day number digits |
| 12-15 | Heart icons (4 × 8×8) |
| 16 | Hovel building 16×16 |
| 17+ | Hidden |

### Hovel upgrade system

- `hovel_level` 0-4 persists across days; resets on new game.
- Cost tables in `source/results.c`:
  - Level 0→1: 20 wood
  - Level 1→2: 30 wood, 15 gold
  - Level 2→3: 45 wood, 25 gold
  - Level 3→4: 60 wood, 40 gold, 20 fish
- Names: TENT / CABIN / COTTAGE / LONGHOUSE / MANOR.

### World map (results.c — draw_world_map)

7 regions drawn in Mode 3 on SELECT: OAK GLADE, MARROW CAVE, MEADOW, PONDSHORE,
WILLOW HOVEL, DRIFTWOOD, CROSSROADS PATH.

### Commits

- `50ae54f` — all Steps 1-7 (HUD, minigames, hearts, satchel, world map, hovel upgrades)
- `b260616` — hat on Willow (tiles 49/50 rows 0-1: gold crown + brim) + hovel OBJ sprite (tiles 55-58)

---

# Dev Log — Day 5

Date: 2026-05-14

- Added warm-gold **hat** to Willow sprite: tiles 49/50 rows 0-1 replaced hair with
  hat crown (colour 9 = gold shadow) and brim (colour 8 = warm gold).
- Added **hovel building OBJ sprite** (tiles 55-58): 16×16 front-facing cabin with
  red pitched roof (colours 6/7), brown walls (C/D), gold window (8/9), dark door (D/transparent).
- Hovel rendered at world tile (26, 16), scrolls with camera, auto-hides off-screen (OAM slot 16).
- Final ROM written as `hearthwood.gba` (36 KB). Commit `b260616` pushed to `main`.

