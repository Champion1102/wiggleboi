# WiggleBoi - Snake Game from Scratch in C

A classic terminal-based Snake game built **entirely from scratch** in C, without using any standard library functions for core logic. No `printf`, no `malloc`, no `strlen` — just raw system calls.

## How It Works

The game is powered by **five custom-built libraries**:

| Library | Purpose | Replaces |
|---------|---------|----------|
| `libs/memory.c` | Memory allocation via `mmap()` syscall | `malloc()` / `free()` |
| `libs/string.c` | String length and integer-to-string conversion | `strlen()` / `sprintf()` |
| `libs/math.c` | Random numbers (LCG), bounds checking, collision detection | `rand()` / math functions |
| `libs/screen.c` | Terminal rendering via ANSI escape codes and `write()` | `printf()` / `ncurses` |
| `libs/keyboard.c` | Non-blocking raw input via `termios` and `select()` | `getchar()` / `scanf()` |

## Build and Run

```bash
make        # compile
./snake     # play!
make clean  # remove binary
```

## Controls

- **W** — Move up
- **A** — Move left
- **S** — Move down
- **D** — Move right
- **Q** — Quit

## Game Rules

- The snake starts in the center moving right
- Eat food (`*`) to grow and increase your score
- Game ends if you hit a wall (`#`) or your own body
- Score is displayed below the game board

## Project Structure

```
wiggleboi/
├── main.c           — game loop and core logic
├── Makefile          — build configuration
└── libs/
    ├── memory.h/c   — mmap/munmap allocator
    ├── string.h/c   — str_len, int_to_str
    ├── math.h/c     — RNG, bounds, collision
    ├── screen.h/c   — ANSI terminal rendering
    └── keyboard.h/c — raw mode input handling
```

## Technical Highlights

- **Zero standard library** for core logic — verified with `nm snake | grep printf` (no matches)
- **Differential rendering** — only 2-3 terminal writes per frame instead of full screen redraw
- **Custom mmap() allocator** — requests memory directly from the OS kernel
- **Flicker-free** — cursor is hidden during gameplay
- **~285 lines of actual code** (excluding comments and blank lines)
