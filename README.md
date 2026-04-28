<div align="center">

<img src="assets/logo.svg" alt="WiggleBoi" width="700" />

<br>

<img src="https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=white" />
<img src="https://img.shields.io/badge/Platform-Terminal-black?style=for-the-badge&logo=windowsterminal&logoColor=white" />
<img src="https://img.shields.io/badge/stdlib-ZERO-ff3333?style=for-the-badge" />
<img src="https://img.shields.io/badge/Binary-~90KB-2FC446?style=for-the-badge" />

<br><br>

<h3>First-person 3D raycasting snake game in the terminal</h3>
<p>Wolfenstein 3D meets Snake. DDA raycasting, half-block pixel rendering, sprite projection — all in pure C with zero standard library.</p>

---

</div>

## What Is This

A snake game built entirely from scratch in C — no `printf`, no `malloc`, no `strlen`, no `math.h`. Just raw system calls (`mmap`, `write`, `read`, `fork`, `select`, `ioctl`). Every component is hand-rolled: the 3D raycaster, the renderer, the allocator, the RNG, the input system, the audio engine.

This branch adds a **first-person 3D mode** — the game launches into a Wolfenstein-style raycasted view where you see the arena from the snake's eyes. Walls have depth and directional coloring, food and power-ups are 3D sprites, and a checkerboard floor gives motion parallax.

The 2D version lives on the `main` branch.

---

## Build & Run

```bash
make clean  # remove old binary
make        # compile
./snake     # play!
```

Requires a **truecolor terminal** (iTerm2, Terminal.app, Kitty, Ghostty, Windows Terminal, most modern Linux terminals).

## Controls (3D Mode)

Controls are **camera-relative** — left/right turn relative to where you're facing:

```
  W / Up Arrow    — Move forward
  A / Left Arrow  — Turn left
  D / Right Arrow — Turn right
  S / Down Arrow  — Reverse direction
  P               — Pause
  Q               — Quit / Back to menu
  ENTER           — Start / Restart
  Left/Right      — Select mode on menu
```

## HUD Elements

- **Crosshair** — small cross at screen center
- **Compass** — `[N]`, `[SE]`, etc. shows facing direction
- **Minimap** — top-right corner shows full arena, player position, and facing direction arrow
- **Score + power-up indicators** — bottom row

---

## 3D Rendering

### Raycaster
- **DDA algorithm** — same grid-traversal as Wolfenstein 3D, using the existing spatial grid for O(1) wall detection
- **Fixed-point math** — 10-bit fractional precision, precomputed sin/cos lookup table (Bhaskara I approximation), no floats anywhere
- **Fisheye correction** — perpendicular distance prevents barrel distortion
- **FOV** — 170 angle units (~60 degrees)
- **View distance** — 40 grid cells with distance fog

### Half-Block Pixel Rendering
Each terminal cell renders **2 vertical pixels** using Unicode half-block characters (`U+2580`). A terminal window of 80x24 becomes an 80x48 pixel framebuffer. Foreground color = top pixel, background color = bottom pixel.

### Wall Types
Walls are color-coded by type:
- **Arena boundary** — blue-tinted (110, 120, 175)
- **Obstacles** — warm stone (160, 130, 100)
- **Snake bodies** — green-tinted (80, 170, 100)

N/S walls are shaded darker than E/W walls (classic Wolfenstein directional lighting).

### Sprites
Food, power-ups, bonus items, and AI snakes are rendered as 3D billboarded sprites with:
- Distance-based fog
- Z-buffer occlusion against walls
- Pulsing animation per entity type

### Floor
Checkerboard pattern computed via floor raycasting — world-space coordinates give motion parallax as the snake moves.

---

## Game Modes

| Mode | Description |
|------|-------------|
| **Classic** | Speed increases with score. The original. |
| **Speed** | Starts fast, gets faster. Pure survival. |
| **Infinity** | Walls wrap around — no wall death. |
| **Challenge** | Obstacles spawn on the grid as you score. |

## Items & Power-ups

| Item | Effect |
|------|--------|
| Red food | +1 point, grow by 1 |
| Golden bonus | +3 points, timed — blinks before vanishing |
| Purple poison | Shrinks snake by 3 segments |
| Blue shield | Survive one collision, bounce off walls |
| Yellow speed | 1.5x game speed for 5 seconds |
| Cyan slow-mo | 0.67x game speed for 5 seconds |
| Orange x2 | Double points for 10 seconds |

## AI Opponents

Up to 4 AI snakes spawn as your score rises (at 8, 16, 25, 35). They compete for your food and create obstacles you must avoid.

---

## Architecture

### File Structure

```
wiggleboi/
  main.c                 Game loop, state machine, 2D/3D rendering (~1200 lines)
  libs/
    ray.h/.c             DDA raycaster, sprite projection, floor casting (~420 lines)
    screen.h/.c          Half-block pixel buffer + cell-mode renderer (~315 lines)
    entity.h/.c          Entity pool + spatial grid
    ai.h/.c              AI snakes with bounded BFS pathfinding
    fx.h/.c              Particle system + screen shake
    keyboard.h/.c        Raw mode input + arrow key parsing
    sound.h/.c           Cross-platform audio via fork+exec (afplay/paplay)
    save.h/.c            Binary high score persistence (~/.wiggleboi_scores)
    memory.h/.c          mmap/munmap allocator
    string.h/.c          str_len, int_to_str
    math.h/.c            LCG random, bounds check
```

### Technical Stats

| Metric | Value |
|--------|-------|
| Total lines of code | ~3,000 |
| Binary size | ~90KB |
| Runtime memory | < 64KB |
| Standard library calls | 0 (only raw syscalls) |
| External dependencies | 0 |
| Rendering | Half-block pixels (2x vertical resolution) |
| Math | Fixed-point (10-bit shift), no floats |
| Platform | macOS, Linux |

---

<div align="center">

```
        Built with raw C and zero chill.

              ~ WiggleBoi 3D ~

                ooooooooooooo@
```

</div>
