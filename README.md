# CHIP-8 Emulator

A fully-functional CHIP-8 emulator written in C using SDL2 for graphics and audio.
This comes from an educational project by ‘Esadecimale’.
This implementation supports the classic CHIP-8 instruction set for compatibility with different ROMs.

## Requirements

- GCC compiler
- SDL2 library

### Installing SDL2

**Debian:**
```bash
sudo apt-get install libsdl2-dev
```

**Fedora:**
```bash
sudo dnf install SDL2-devel
```

**Arch Linux:**
```bash
sudo pacman -S sdl2
```

## Usage

Run the emulator with a CHIP-8 ROM file:

```bash
./chip8 <path-to-rom>
```

Example:
```bash
./chip8 ../roms/PONG.ch8
```

## Technical Details

- **Memory**: 4KB (4096 bytes) total
- **Registers**: 16 general-purpose 8-bit registers (V0-VF), VF used as flag
- **Display**: 64x32 monochrome pixels
- **Stack**: 16 levels for subroutine calls
- **Timers**: Delay and sound timers at 60 Hz
- **Font**: Built-in 4x5 hex font for characters 0-F
- **Program Start**: ROMs loaded at address `0x200`
- **Screen Scale**: 15x (configurable in `main.c`)

