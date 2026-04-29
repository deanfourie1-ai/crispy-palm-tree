/*
 * gba.h  –  GBA hardware register definitions, types and utility macros.
 *
 * Covers display control, backgrounds, OBJ/sprite attributes, key input,
 * palette/VRAM/OAM pointers and basic colour helpers.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* ── Portable integer types ──────────────────────────────────────────────── */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

/* ── Display / video registers ───────────────────────────────────────────── */
#define REG_DISPCNT   (*(volatile u16 *)0x04000000)
#define REG_DISPSTAT  (*(volatile u16 *)0x04000004)
#define REG_VCOUNT    (*(volatile u16 *)0x04000006)

/* Background control registers */
#define REG_BG0CNT    (*(volatile u16 *)0x04000008)
#define REG_BG1CNT    (*(volatile u16 *)0x0400000A)

/* Background scroll registers (write-only) */
#define REG_BG0HOFS   (*(volatile u16 *)0x04000010)
#define REG_BG0VOFS   (*(volatile u16 *)0x04000012)
#define REG_BG1HOFS   (*(volatile u16 *)0x04000014)
#define REG_BG1VOFS   (*(volatile u16 *)0x04000016)

/* Key input register – bit = 0 means pressed */
#define REG_KEYINPUT  (*(volatile u16 *)0x04000130)

/* ── DISPCNT bit flags ───────────────────────────────────────────────────── */
#define DCNT_MODE0    (0)
#define DCNT_BG0      (1 << 8)
#define DCNT_BG1      (1 << 9)
#define DCNT_OBJ      (1 << 12)
#define DCNT_OBJ_1D   (1 << 6)   /* 1-D OBJ tile mapping */

/* ── Key bits ────────────────────────────────────────────────────────────── */
#define KEY_A         (1 << 0)
#define KEY_B         (1 << 1)
#define KEY_SELECT    (1 << 2)
#define KEY_START     (1 << 3)
#define KEY_RIGHT     (1 << 4)
#define KEY_LEFT      (1 << 5)
#define KEY_UP        (1 << 6)
#define KEY_DOWN      (1 << 7)
#define KEY_R         (1 << 8)
#define KEY_L         (1 << 9)

/* ── Background control helpers ─────────────────────────────────────────── */
/* Character (tile-graphics) base block – each block is 16 KB */
#define BG_CBB(n)        ((n) << 2)
/* Screen (tile-map) base block – each block is 2 KB */
#define BG_SBB(n)        ((n) << 8)
/* Colour depth */
#define BG_4BPP          (0)
#define BG_8BPP          (1 << 7)
/* Screen size for regular BGs */
#define BG_SIZE_32x32    (0 << 14)
#define BG_SIZE_64x32    (1 << 14)
#define BG_SIZE_32x64    (2 << 14)
#define BG_SIZE_64x64    (3 << 14)
/* Priority */
#define BG_PRIO(n)       (n)

/* ── VRAM / memory pointers ──────────────────────────────────────────────── */
/* BG palette (256 × u16, 512 bytes) */
#define MEM_PAL_BG       ((volatile u16 *)0x05000000)
/* OBJ palette (256 × u16, 512 bytes, immediately after BG palette) */
#define MEM_PAL_OBJ      ((volatile u16 *)0x05000200)

/* VRAM base (96 KB total) */
#define MEM_VRAM         ((volatile u8  *)0x06000000)

/* Character base blocks for BG tiles (4 blocks × 16 KB each = 64 KB) */
#define CBB_BASE(n)      ((volatile u16 *)(0x06000000 + (n) * 0x4000))
/* Screen base blocks for BG maps  (32 blocks × 2 KB each = 64 KB) */
#define SBB_BASE(n)      ((volatile u16 *)(0x06000000 + (n) * 0x0800))

/* OBJ tile memory starts at VRAM + 64 KB (in modes 0/1/2)              */
/* Each 4-bpp 8×8 tile = 32 bytes; tiles indexed from 0                 */
#define MEM_OBJ_TILES    ((volatile u8  *)0x06010000)

/* OAM – up to 128 sprite attribute entries (each 8 bytes) */
#define MEM_OAM          ((volatile u16 *)0x07000000)

/* ── OBJ (sprite) attribute helpers ──────────────────────────────────────  */
typedef struct {
    u16 attr0;
    u16 attr1;
    u16 attr2;
    s16 pad;
} OBJ_ATTR;

#define OAM_MEM          ((volatile OBJ_ATTR *)0x07000000)

/* attr0 */
#define OBJ_Y(y)          ((y) & 0xFF)
#define OBJ_SHAPE_SQ      (0 << 14)
#define OBJ_4BPP          (0 << 13)
#define OBJ_HIDE          (2 << 8)   /* obj mode = 10b → disabled */

/* attr1 */
#define OBJ_X(x)          ((x) & 0x1FF)
#define OBJ_SIZE_8        (0 << 14)  /* with SHAPE_SQ → 8×8   */
#define OBJ_SIZE_16       (1 << 14)  /* with SHAPE_SQ → 16×16 */

/* attr2 */
#define OBJ_TILE(t)       ((t) & 0x3FF)
#define OBJ_PAL(p)        ((p) << 12)
#define OBJ_PRIO(p)       ((p) << 10)

/* ── Colour helper ───────────────────────────────────────────────────────── */
/* Each channel 0-31 (5 bits); result is a 15-bit GBA colour word */
#define GBA_RGB(r, g, b)  ((r) | ((g) << 5) | ((b) << 10))

/* ── Screen dimensions ───────────────────────────────────────────────────── */
#define SCREEN_W          240
#define SCREEN_H          160

/* ── VBlank wait ─────────────────────────────────────────────────────────── */
static inline void vblank_wait(void)
{
    while ((REG_DISPSTAT & (1 << 0)) == 0) {}  /* wait for VBlank start */
    while ((REG_DISPSTAT & (1 << 0)) != 0) {}  /* wait for VBlank end   */
}

/* ── Simple memory utilities (no CRT, no libc) ───────────────────────────── */
static inline void memset16(volatile u16 *dst, u16 val, u32 count)
{
    while (count--) *dst++ = val;
}
static inline void memcpy16(volatile u16 *dst, const u16 *src, u32 count)
{
    while (count--) *dst++ = *src++;
}
static inline void memcpy8(volatile u8 *dst, const u8 *src, u32 count)
{
    while (count--) *dst++ = *src++;
}
