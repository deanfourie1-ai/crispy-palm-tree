# ============================================================
# Makefile  –  GBA RTS base game
#
# Requires: gcc-arm-none-eabi, binutils-arm-none-eabi
# Install:  sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
#
# Targets:
#   make          – build warcraftgba.gba
#   make clean    – remove build artefacts
# ============================================================

TARGET   := warcraftgba
BUILD    := build
SOURCES  := source

# Source files
C_FILES  := $(wildcard $(SOURCES)/*.c)
S_FILES  := $(wildcard $(SOURCES)/*.s)
OBJ_C    := $(patsubst $(SOURCES)/%.c, $(BUILD)/%.o, $(C_FILES))
OBJ_S    := $(patsubst $(SOURCES)/%.s, $(BUILD)/%.o, $(S_FILES))
OBJECTS  := $(OBJ_S) $(OBJ_C)     # startup object first

# Tool-chain
PREFIX   := arm-none-eabi-
CC       := $(PREFIX)gcc
AS       := $(PREFIX)as
LD       := $(PREFIX)gcc
OBJCOPY  := $(PREFIX)objcopy

# Compiler flags
#   -mcpu=arm7tdmi      GBA processor
#   -mthumb-interwork   allow interworking between ARM and Thumb code
#   -fno-common         prevent common-symbol merging
#   -fno-strict-aliasing safe pointer casts through volatile I/O registers
ARCH     := -mcpu=arm7tdmi -mthumb-interwork
CFLAGS   := $(ARCH) -O2 -Wall -Wextra \
            -fno-common -fno-strict-aliasing \
            -ffunction-sections -fdata-sections \
            -I$(SOURCES)
ASFLAGS  := $(ARCH)

# Linker flags
#   -nostartfiles  we provide our own crt0.s
#   -nostdlib      no C standard library
#   -T             our custom linker script
LDFLAGS  := $(ARCH) -nostartfiles -nostdlib \
            -T gba_cart.ld \
            -Wl,--gc-sections

# ── Rules ─────────────────────────────────────────────────────────────────
.PHONY: all clean

all: $(TARGET).gba

# Create build directory
$(BUILD):
	mkdir -p $(BUILD)

# Compile C sources
$(BUILD)/%.o: $(SOURCES)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .s sources
$(BUILD)/%.o: $(SOURCES)/%.s | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

# Link ELF
$(BUILD)/$(TARGET).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@

# Strip to raw binary
$(TARGET).gba: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@echo ""
	@echo "  Built: $(TARGET).gba  ($$(wc -c < $(TARGET).gba) bytes)"

clean:
	rm -rf $(BUILD) $(TARGET).gba
