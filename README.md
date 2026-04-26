<div align="center">

<img src="assets/logo.svg" alt="WiggleBoi" width="700" />

<br>

<img src="https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=white" />
<img src="https://img.shields.io/badge/Platform-Terminal-black?style=for-the-badge&logo=windowsterminal&logoColor=white" />
<img src="https://img.shields.io/badge/stdlib-ZERO-ff3333?style=for-the-badge" />
<img src="https://img.shields.io/badge/Binary-72KB-2FC446?style=for-the-badge" />

<br><br>

<h3>The world's most overengineered terminal snake game</h3>
<p>Truecolor rendering. Entity pools. AI opponents. Particle effects. All in pure C with zero standard library.</p>

---

</div>

## What Is This

A snake game built entirely from scratch in C — no `printf`, no `malloc`, no `strlen`. Just raw system calls (`mmap`, `write`, `read`, `fork`, `select`, `ioctl`). Every component is hand-rolled: the renderer, the allocator, the RNG, the input system, the audio engine.

It started as a Nokia-style snake clone. It became a truecolor, double-buffered, entity-pooled, AI-driven, particle-spewing terminal game with 4 game modes, power-ups, and persistent high scores — in 72KB.

---

## Play

```bash
make        # compile
./snake     # play!
make clean  # remove binary
```

Requires a truecolor terminal (iTerm2, Terminal.app, Kitty, Ghostty, Windows Terminal, most modern Linux terminals).

## Controls

```
  WASD / Arrow Keys / HJKL — Move
  P — Pause
  Q — Quit / Back to menu
  ENTER — Start / Restart
  Left/Right — Select mode on menu
```

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

| Personality | Color | Behavior |
|-------------|-------|----------|
| Aggressive | Red/orange | BFS pathfinds toward food |
| Cautious | Blue/purple | Avoids player when close |
| Random | Yellow/green | Pure random wander |

AI pathfinding uses depth-limited BFS (max 5 cells) with distributed updates — only 1 AI runs expensive pathfinding per tick.

---

## Architecture

### Rendering Engine
- **Double-buffered truecolor** — 24-bit RGB via `\033[48;2;R;G;Bm`
- **Dirty-cell diffing** — only changed cells are written each frame
- **Single `write()` per frame** — all ANSI output accumulated in staging buffer
- **Synchronized output** — `\033[?2026h/l` prevents tearing
- Each game cell = 2 terminal columns (approximate square aspect ratio)

### Entity System (OpenRCT2-inspired)
- **Fixed-size pool** — 1024 pre-allocated entities, stack-based free list
- **Spatial grid** — `grid[y * w + x]` gives O(1) collision detection
- **Packed structs** — 12 bytes per entity with linked-list chaining for snake bodies
- Snake movement = push head + pop tail on the linked chain (no array shifting)

### Optimization Techniques

| Source | Technique | Application |
|--------|-----------|-------------|
| OpenRCT2 | Entity pool with free list | O(1) alloc/free, zero fragmentation |
| OpenRCT2 | Spatial grid O(1) lookup | Replaces O(n) body collision scan |
| OpenRCT2 | Distributed tick updates | 1 AI gets BFS per tick via `idx & 3 == tick & 3` |
| OpenRCT2 | Packed structs with bit flags | 12-byte entities, single-byte flags |
| Terminal-doom | Static buffers everywhere | All buffers fixed-size, zero allocation in hot loop |
| Terminal-doom | Synchronized output mode 2026 | Tear-free rendering |
| Terminal-doom | Single write() per frame | Minimized syscall overhead |

---

## 11 Custom Libraries — Zero Dependencies

```
wiggleboi/
  main.c                 Game loop, state machine, rendering (~1000 lines)
  libs/
    memory.h/.c          mmap/munmap allocator (replaces malloc/free)
    string.h/.c          str_len, int_to_str (replaces strlen/sprintf)
    math.h/.c            LCG random, bounds check (replaces rand/math.h)
    screen.h/.c          Double-buffered truecolor renderer (replaces ncurses)
    keyboard.h/.c        Raw mode input + arrow key parsing (replaces getchar)
    sound.h/.c           Cross-platform audio via fork+exec (afplay/paplay)
    entity.h/.c          Entity pool + spatial grid (OpenRCT2 pattern)
    fx.h/.c              Particle system + screen shake
    ai.h/.c              AI snakes with bounded BFS pathfinding
    save.h/.c            Binary high score persistence (~/.wiggleboi_scores)
```

## Technical Stats

| Metric | Value |
|--------|-------|
| Total lines of code | ~2,400 |
| Binary size | 72KB |
| Runtime memory | < 64KB |
| Standard library calls | 0 (only raw syscalls) |
| External dependencies | 0 |
| Game modes | 4 |
| AI opponents | Up to 4 simultaneous |
| Color depth | 24-bit truecolor (16.7M colors) |
| Platform | macOS, Linux |

---

<div align="center">

```
        Built with raw C and zero chill.

              ~ WiggleBoi 2025 ~

                ooooooooooooo@
```

</div>
