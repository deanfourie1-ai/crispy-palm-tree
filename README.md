# crispy-palm-tree – GBA RTS Base

A Game Boy Advance ROM project: a simplified real-time strategy game inspired by
classic fantasy RTS titles.

## Game concept

* Tile-based, scrollable **64 × 32 tile map** (512 × 256 px) with five terrain
  types: Grass, Water, Forest, Mountain and Dirt.
* **Player units** – 3 Footmen (blue) + 1 Peasant (brown) – start in the
  south-west of the map.
* **Enemy units** – 3 Orcs (green/red) – start in the north-east.
* A **yellow cursor** indicates the current tile under the player's control.
* **Map Select foundation** is in place: New Game opens a map list that
  currently contains **MAP 1 (PLACEHOLDER)**.
* Map entries are now metadata-driven (name, description, preview theme), so
  adding future maps is mostly a data-table update plus map terrain data.

## Controls

| Button | Action |
|--------|--------|
| D-Pad | Move cursor / camera follows |
| **A** | Select player unit at cursor, OR issue a move command when a unit is already selected, OR attack an enemy at the cursor tile |
| **B** | Deselect current unit / cancel |
| **R** | Open build menu (Wood Depot, Gold Mine, Food Store), then place at cursor |

Menu flow:

* Select **NEW GAME** to open the map list.
* Choose **MAP 1 (PLACEHOLDER)** and press **A** to start.

## Screenshot preview

![GBA game preview](preview.png)

> Preview rendered from palette and tile data – shows the 240 × 160 GBA screen
> at 3× scale.

## Building

### Requirements

```
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
```

### Compile

```bash
make
```

This produces **`embercrown.gba`** (a valid GBA ROM binary, ~5–6 KB).

### Play

Open `embercrown.gba` in any GBA emulator that supports `.gba` ROMs, such as:

* [mGBA](https://mgba.io/) *(recommended)*
* [VisualBoyAdvance-M](https://vba-m.com/)
* [RetroArch](https://www.retroarch.com/) with the mGBA core

```bash
mgba embercrown.gba
```

## Project structure

```
.
├── Makefile              Build system (arm-none-eabi-gcc)
├── gba_cart.ld           GBA cartridge linker script
├── include/
│   ├── gba.h             Hardware register definitions, types, helpers
│   ├── tiles.h           BG tile and OBJ sprite pixel data + palettes
│   ├── video.h           Display API
│   ├── input.h           Input API
│   ├── map.h             Map API
│   ├── unit.h            Unit API
│   ├── game.h            Game logic API
│   └── menu.h            Menu API
├── source/
│   ├── crt0.s            ARM startup code + GBA ROM header
│   ├── video.c           Display initialisation (VRAM, palette upload)
│   ├── input.c           Button debounce and edge detection
│   ├── map.c             Tile map data and BG scroll management
│   ├── unit.c            Unit state, tile-step movement, OAM rendering
│   ├── game.c            Game logic: cursor, selection, move commands
│   ├── menu.c            Main menu UI and navigation
│   └── main.c            Entry point and main loop
├── build/                Generated objects, deps, and ELF (auto-generated)
├── test_builds/          Archived older test ROMs/SAVs
└── preview.png           Simulated screen preview
```

## Roadmap

The current commit establishes the playable base.  Planned additions:

- [ ] Simple enemy AI that walks toward player units
- [ ] HP bars rendered above each unit
- [ ] Resource collection (Peasant gathers gold from a Forest tile)
- [ ] Sound effects via GBA Direct Sound
- [ ] Multiple maps with a simple save/load system
