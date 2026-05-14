# Compile Instructions — Hearthwood Hollow (Windows + WSL Ubuntu)

This project builds a Game Boy Advance ROM (`hearthwood.gba`) using
`arm-none-eabi-gcc` from WSL Ubuntu.

---

## One-time setup

1. Verify WSL and Ubuntu are available:
```powershell
wsl --status
wsl -l -v
```

2. Install required build tools inside Ubuntu:
```powershell
wsl -d Ubuntu -- bash -c "sudo apt update && sudo apt install -y build-essential make gcc-arm-none-eabi binutils-arm-none-eabi"
```

---

## Build the ROM

Run from Windows PowerShell at the repository root:

```powershell
wsl -d Ubuntu -- bash -c "cd '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree' && make 2>&1"
```

### Clean rebuild

> Close the emulator first — `make clean` deletes `hearthwood.gba` and will fail
> with "Permission denied" if the file is open.

```powershell
wsl -d Ubuntu -- bash -c "cd '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree' && make clean && make 2>&1"
```

---

## Output

Successful builds produce:
- `build/hearthwood.elf`  — ELF binary (intermediate)
- `hearthwood.gba`        — final GBA ROM (~36 KB)

### If `hearthwood.gba` is locked (emulator open)

The ELF is always written. Convert it manually:
```powershell
wsl -d Ubuntu -- bash -c "arm-none-eabi-objcopy -O binary '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree/build/hearthwood.elf' '/mnt/c/Users/rosel/Documents/AI Projects/crispy-palm-tree/hearthwood.gba'"
```

---

## Test in emulator

1. Open [mGBA](https://mgba.io/) (recommended).
2. Load `hearthwood.gba` from the project root.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `make clean` fails with Permission denied | Close the emulator, then retry |
| `arm-none-eabi-gcc: command not found` | Run the apt install step above |
| `sudo` prompts for password | Enter your WSL Ubuntu user password |
| Build succeeds but `.gba` not updated | Use the manual `objcopy` command above |
