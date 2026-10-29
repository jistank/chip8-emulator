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

To build the executable, go in src/ and digit: 
```bash
make
```

Run the emulator with a CHIP-8 ROM file (you can find different ROMs in the roms/ folder):

```bash
./chip8 <path-to-rom>
```

Example in a single command:
```bash
make && ./chip8 ../roms/Invaders.c8
```
## Controls

### Pong
| Key | Action |
|-----|--------|
| `1` | Move up |
| `q` | Move down |

### Tetris
| Key | Action |
|-----|--------|
| `a` | Move down faster |
| `e` | Move right |
| `w` | Move left |
| `q` | Rotate |

### Space Invaders
| Key | Action |
|-----|--------|
| `e` | Move right |
| `w` | Move left |
| `q` | Shoot |

## Technical Details

- **Memory**: 4KB (4096 bytes) total
- **Registers**: 16 general-purpose 8-bit registers (V0-VF), VF used as flag
- **Display**: 64x32 monochrome pixels
- **Stack**: 16 levels for subroutine calls
- **Timers**: Delay and sound timers at 60 Hz
- **Font**: Built-in 4x5 hex font for characters 0-F
- **Program Start**: ROMs loaded at address `0x200`
- **Screen Scale**: 15x (configurable in `main.c`)

