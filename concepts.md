# WiggleBoi — Concepts & Theory

A deep reference covering every algorithm, data structure, and technique used in this project. Written to answer the question: "How does this work, and why was it built this way?"

---

## Table of Contents

1. [Bitwise Arithmetic](#1-bitwise-arithmetic)
2. [Fixed-Point Mathematics](#2-fixed-point-mathematics)
3. [Trigonometry Without math.h](#3-trigonometry-without-mathh)
4. [Memory Management Without malloc](#4-memory-management-without-malloc)
5. [Entity Pool & Free-List Allocator](#5-entity-pool--free-list-allocator)
6. [Spatial Grid Collision Detection](#6-spatial-grid-collision-detection)
7. [Terminal Rendering & Double Buffering](#7-terminal-rendering--double-buffering)
8. [Half-Block Pixel Rendering](#8-half-block-pixel-rendering)
9. [DDA Raycasting Algorithm](#9-dda-raycasting-algorithm)
10. [Fisheye Correction](#10-fisheye-correction)
11. [Texture Mapping in a Raycaster](#11-texture-mapping-in-a-raycaster)
12. [Floor & Ceiling Casting](#12-floor--ceiling-casting)
13. [Sprite Projection (Billboard Rendering)](#13-sprite-projection-billboard-rendering)
14. [Procedural Texture Generation](#14-procedural-texture-generation)
15. [Lighting & Fog Model](#15-lighting--fog-model)
16. [Linear Congruential Generator (RNG)](#16-linear-congruential-generator-rng)
17. [BFS Pathfinding for AI](#17-bfs-pathfinding-for-ai)
18. [State Machine Pattern](#18-state-machine-pattern)
19. [Non-Blocking Input & Escape Sequences](#19-non-blocking-input--escape-sequences)
20. [Process-Based Audio (fork + exec)](#20-process-based-audio-fork--exec)
21. [Particle System](#21-particle-system)
22. [Snake as a Doubly-Linked List](#22-snake-as-a-doubly-linked-list)

---

## 1. Bitwise Arithmetic

**File:** `libs/math.c`

All five basic arithmetic operations are built from scratch using only bitwise operators: `&`, `|`, `^`, `~`, `<<`, `>>`.

### Addition — Ripple Carry Adder

The key insight: XOR gives the sum of two bits ignoring carry, and AND identifies where carries occur.

```
Example: 5 + 3  (binary: 101 + 011)

  Iteration 1:
    carry = 101 & 011 = 001
    a     = 101 ^ 011 = 110
    b     = 001 << 1  = 010

  Iteration 2:
    carry = 110 & 010 = 010
    a     = 110 ^ 010 = 100
    b     = 010 << 1  = 100

  Iteration 3:
    carry = 100 & 100 = 100
    a     = 100 ^ 100 = 000  (wait, this is wrong — let me redo)
```

Actually, let's trace correctly:

```
5 + 3 = 8

  a=101, b=011
  carry = 101 & 011 = 001    (carry at bit 0)
  a     = 101 ^ 011 = 110    (sum without carry)
  b     = 001 << 1  = 010    (carry shifted left)

  a=110, b=010
  carry = 110 & 010 = 010    (carry at bit 1)
  a     = 110 ^ 010 = 100    (sum without carry)
  b     = 010 << 1  = 100    (carry shifted left)

  a=100, b=100
  carry = 100 & 100 = 100    (carry at bit 2)
  a     = 100 ^ 100 = 000
  b     = 100 << 1  = 1000

  a=000, b=1000
  carry = 0
  a     = 1000
  b     = 0           → STOP

  Result: 1000 = 8 ✓
```

**Time complexity:** O(32) worst case — carry can propagate at most 32 bits.

### Negation — Two's Complement

`-x = ~x + 1`

Flip all bits then add one. This is how hardware represents negative numbers.

```
Example: negate 5
  5     = 00000101
  ~5    = 11111010
  ~5+1  = 11111011 = -5 in two's complement ✓
```

### Subtraction

`a - b = a + (-b)` — just negate and add.

### Multiplication — Shift-and-Add

Binary long multiplication: for each set bit in `b`, add a shifted copy of `a`.

```
Example: 6 × 5  (110 × 101)

  b has bit 0 set (1): result += 110    → result = 110
  shift a left:        a = 1100
  b has bit 1 clear:   skip
  shift a left:        a = 11000
  b has bit 2 set (1): result += 11000  → result = 11110 = 30 ✓
```

**Sign handling:** Make both operands positive, multiply, then negate result if exactly one operand was negative (`neg ^= 1` tracks this).

### Division — Binary Long Division (Restoring)

Process dividend bits from MSB to LSB. Shift remainder left, append next dividend bit. If remainder >= divisor, subtract and set quotient bit.

```
Example: 13 / 3  (1101 / 11)

  i=3: r=0, append bit 3 of 1101 → r=1.  1 < 3 → q bit=0
  i=2: r=10, append bit 2        → r=11.  3 >= 3 → subtract, r=0, q bit=1
  i=1: r=0, append bit 1         → r=1.   1 < 3 → q bit=0
  i=0: r=10, append bit 0        → r=11.  3 >= 3 → subtract, r=0, q bit=1

  quotient = 0101 = 4 (with remainder 1) ✓   (13 = 4×3 + 1)
```

### Modulo

`a % b = a - (a / b) * b` — composed from division, multiplication, subtraction.

### Why This Matters

These functions prove that all arithmetic reduces to bit manipulation. The CPU does exactly this in hardware — our code mirrors the logic gates of an ALU (Arithmetic Logic Unit).

---

## 2. Fixed-Point Mathematics

**Files:** `libs/ray.c`, `main.c`

### The Problem

We need sub-integer precision (camera at position 5.5, angles like 37.2 degrees) but avoid floating-point for portability, speed, and the no-library constraint.

### The Solution: Fixed-Point

Reserve the lower N bits of an integer as a fractional part.

```
FP_SHIFT = 10
FP_ONE   = 1 << 10 = 1024

Integer 1    → stored as 1024
Integer 5    → stored as 5120
Half (0.5)   → stored as 512
Position 3.7 → stored as 3 × 1024 + 717 ≈ 3789
```

### Operations

| Operation     | Fixed-Point Formula                         |
|---------------|---------------------------------------------|
| Addition      | `a + b` (no change)                         |
| Subtraction   | `a - b` (no change)                         |
| Multiplication| `(a * b) >> FP_SHIFT`                       |
| Division      | `(a << FP_SHIFT) / b`                       |
| Int → FP      | `n << FP_SHIFT` or `n * FP_ONE`             |
| FP → Int      | `n >> FP_SHIFT`                             |
| Fractional    | `n & (FP_ONE - 1)`                          |

### Why `(long long)` in fp_mul?

`fp_mul(a, b)` casts to `long long` before multiplying because two 32-bit fixed-point values multiplied can exceed 32-bit range:

```c
int fp_mul(int a, int b) {
    return (int)(((long long)a * b) >> FP_SHIFT);
}
```

Example: `fp_mul(5120, 3072)` = 5 × 3 = 15 in fixed point.
Raw: 5120 × 3072 = 15,728,640 → shift right 10 → 15,360 = 15.0 ✓

Without `long long`: 5120 × 3072 = 15,728,640 which fits in 32-bit, but larger values would overflow.

---

## 3. Trigonometry Without math.h

**File:** `libs/ray.c`, function `build_tables()`

### Bhaskara I's Sine Approximation (7th century CE)

Instead of Taylor series or CORDIC, we use an ancient Indian mathematician's rational approximation:

```
sin(x) ≈ 16x(π - x) / (5π² - 4x(π - x))
```

where x is in [0, π].

**Accuracy:** Maximum error ~0.16% — more than enough for a raycaster.

### Integer Adaptation

We map 0..π to 0..512 (ANGLE_180), so:

```c
num = 16 * i * (512 - i);
den = 5 * 512 * 512 - 4 * i * (512 - i);
sin_tab[i] = num * 1024 / den;  // result in fixed-point
```

### Building the Full Table

1. Compute sin for angles 0 to 179 (0° to ~180°) using Bhaskara
2. Mirror for 180-359: `sin[i] = -sin[i - 180]` (sine is antisymmetric)
3. Derive cosine: `cos[i] = sin[i + 90]` (cosine leads sine by 90°)

### Angle System

- 1024 units = 360° (power of 2 for fast wrapping via `& 1023`)
- 256 units = 90°
- This means 1 unit ≈ 0.35°

---

## 4. Memory Management Without malloc

**File:** `libs/memory.c`

### mmap — Direct OS Memory Allocation

```c
void *mem_alloc(unsigned long size) {
    void *p = mmap(0, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON, -1, 0);
    return (p == MAP_FAILED) ? 0 : p;
}
```

| Parameter            | Meaning                                         |
|----------------------|-------------------------------------------------|
| `0`                  | Let the kernel choose the address                |
| `PROT_READ\|WRITE`   | Memory is readable and writable                  |
| `MAP_PRIVATE`        | Changes are private to this process              |
| `MAP_ANON`           | Not backed by a file — pure anonymous memory     |
| `-1, 0`              | No file descriptor, no offset                    |

### Why Not malloc?

`malloc` is part of the C standard library (libc). Our constraint is zero library dependencies. `mmap` is a raw system call — it talks directly to the kernel.

### Trade-off

`mmap` allocates in page-sized chunks (4KB minimum). Fine for large buffers (screen: ~80KB, textures: ~100KB), wasteful for small allocations. That's why entities use a fixed pool instead.

---

## 5. Entity Pool & Free-List Allocator

**File:** `libs/entity.c`

### Problem

The game creates/destroys entities constantly (food eaten, snake grows, obstacles spawn). We need fast allocation without malloc.

### Solution: Fixed-Size Pool with Stack-Based Free List

```
pool[1024]       — pre-allocated entity array (fixed size, never grows)
free_stack[1024] — indices of available slots
free_top         — stack pointer

Allocate: pop index from stack top     → O(1)
Free:     push index back onto stack   → O(1)
```

### Visual Example

```
Initial state:
  free_stack = [1023, 1022, 1021, ..., 2, 1, 0]
  free_top = 1024

After allocating 3 entities:
  free_stack = [1023, 1022, 1021, ..., 2, 1, 0]
                                              ^ free_top = 1021
  Allocated IDs: 0, 1, 2

Free entity 1:
  free_stack[1021] = 1
  free_top = 1022

Next allocation returns ID 1 (reused!)
```

### Entity Structure (10 bytes)

```c
struct Entity {
    unsigned char x, y;              // grid position
    unsigned char type;              // SNAKE_HEAD, FOOD, etc.
    unsigned char flags;             // ENT_ALIVE, ENT_VISIBLE
    unsigned char chain_next, prev;  // linked list for snakes
    unsigned char color_r, g, b;     // RGB color
    unsigned char timer;             // for temporary objects
    unsigned char owner;             // which AI owns this
};
```

### Why This Pattern?

- Zero fragmentation (no free/alloc gaps)
- Cache-friendly (contiguous memory)
- Bounded memory (1024 entities max = 10KB)
- Same pattern used in OpenRCT2 (theme park game engine)

---

## 6. Spatial Grid Collision Detection

**File:** `libs/entity.c`

### Problem

"Is there something at position (5, 3)?" Scanning all entities is O(n). With hundreds of snake segments, this is too slow every frame.

### Solution: 2D Lookup Grid

```
grid[y * width + x] = entity_id   (or ENT_NONE if empty)
```

One byte per cell. For a 39x20 grid = 780 bytes.

### Operations — All O(1)

| Function          | What it does                    |
|-------------------|---------------------------------|
| `grid_set(x,y,id)` | Place entity at position       |
| `grid_clear(x,y)`  | Remove entity from position    |
| `grid_at(x,y)`     | Get entity ID at position      |
| `grid_occupied(x,y)`| Check if position has entity  |

### Consistency Rule

Every time an entity moves, TWO updates must happen:
```
grid_clear(old_x, old_y);
grid_set(new_x, new_y, entity_id);
```

If you forget one, the grid becomes stale and collisions break.

---

## 7. Terminal Rendering & Double Buffering

**File:** `libs/screen.c`

### How Terminal Color Works

ANSI escape sequences control terminal colors:

```
\033[48;2;R;G;Bm   — set background to RGB
\033[38;2;R;G;Bm   — set foreground to RGB
\033[H              — move cursor to top-left
\033[y;xH           — move cursor to row y, column x
```

This is "truecolor" (24-bit) — 16.7 million colors.

### Double Buffering

```
back[]  — the frame being built (write here)
front[] — what's currently on screen

scr_present():
  for each cell:
    if back[i] != front[i]:
      emit ANSI codes to update this cell
      front[i] = back[i]
```

### Why Differential Rendering?

Sending the entire screen every frame = ~80 cells × 50 bytes ANSI ≈ 4KB per frame. But most frames only change ~5% of cells. Diffing reduces output to ~200 bytes per frame, making the game smooth even over SSH.

### Synchronized Output

```
\033[?2026h  — begin synchronized update (terminal holds display)
...render...
\033[?2026l  — end synchronized update (terminal refreshes atomically)
```

This prevents half-drawn frames (tearing).

### Performance Trick: Single write() Syscall

All ANSI output is accumulated in a 65KB buffer (`wbuf`), then flushed with one `write(1, wbuf, len)` call. System calls are expensive — batching minimizes overhead.

---

## 8. Half-Block Pixel Rendering

**File:** `libs/screen.c`, function `scr_pixels_flush()`

### The Problem

A terminal cell is roughly twice as tall as it is wide. If we use one cell = one pixel, vertical resolution is halved.

### The Solution: Unicode Half-Block Character

The character `▀` (U+2580, upper half block) fills the top half of a cell. By setting:
- **Foreground color** = top pixel
- **Background color** = bottom pixel

One terminal cell displays **two vertical pixels**.

```
Terminal cell:
  ┌──────┐
  │ ▀▀▀▀ │  ← foreground color (top pixel)
  │      │  ← background color (bottom pixel)
  └──────┘
```

### Resolution Doubling

```
Terminal: 80 columns × 30 rows
Pixel buffer: 80 wide × 60 tall (30 × 2)
```

### Flush Algorithm

```c
for each terminal row (ty):
    top_pixel = pixels[ty * 2]
    bot_pixel = pixels[ty * 2 + 1]

    if top == bot:
        just set background color (space character)
    else:
        set foreground = top, background = bot
        print '▀' character
```

---

## 9. DDA Raycasting Algorithm

**File:** `libs/ray.c`, function `ray_cast()`

### What is Raycasting?

Cast one ray per screen column from the player into the 2D map. When the ray hits a wall, calculate the distance. Closer walls are drawn taller, farther walls shorter. This creates a 3D perspective illusion from a 2D map — the same technique used in Wolfenstein 3D (1992).

### DDA — Digital Differential Analyzer

Instead of stepping the ray in tiny increments (slow, imprecise), DDA jumps directly from one grid line to the next.

```
Player at (2.3, 1.7), ray going right and slightly up:

  Step 1: Ray crosses vertical grid line x=3  (distance: 0.7/cos)
  Step 2: Ray crosses horizontal grid line y=1 (distance: 0.7/sin)
  Step 3: Compare — which crossing is closer?
          → Jump to the closer one
          → Check if that grid cell is a wall
          → If not, continue to next crossing
```

### Setup Phase

For each ray:
1. **delta_dist_x** = distance between consecutive vertical grid lines along the ray = `|1 / ray_dx|`
2. **delta_dist_y** = distance between consecutive horizontal grid lines = `|1 / ray_dy|`
3. **side_dist_x** = distance from camera to first vertical grid line
4. **side_dist_y** = distance from camera to first horizontal grid line

### Main Loop

```
while no wall hit and depth < MAX_DEPTH:
    if side_dist_x < side_dist_y:
        step in X direction
        side_dist_x += delta_dist_x
        side = 0 (hit vertical face — N/S wall)
    else:
        step in Y direction
        side_dist_y += delta_dist_y
        side = 1 (hit horizontal face — E/W wall)

    check grid cell for wall
```

### Perpendicular Distance

After hitting a wall, the distance used for rendering is NOT the Euclidean distance (that causes fisheye). Instead:

```
if hit vertical face (side=0):
    perp_dist = side_dist_x - delta_dist_x
else:
    perp_dist = side_dist_y - delta_dist_y
```

This gives the perpendicular distance from the camera plane to the wall.

---

## 10. Fisheye Correction

**File:** `libs/ray.c`, line 197-201

### The Problem

Rays at the edge of the FOV travel farther than center rays to reach the same wall. Without correction, walls appear curved outward (barrel distortion / "fisheye effect").

### The Fix

Multiply the perpendicular distance by the cosine of the angle difference between the ray and the camera's forward direction:

```
corrected_dist = raw_dist × cos(ray_angle - camera_angle)
```

Rays at the center: `cos(0) = 1.0` → no change.
Rays at the edge: `cos(±35°) ≈ 0.82` → distance shortened → wall drawn taller → compensates for the longer ray path.

---

## 11. Texture Mapping in a Raycaster

**File:** `libs/ray.c`

### Wall Texture X Coordinate

When a ray hits a wall, we need to know WHERE on the wall face it hit (left edge? middle? right edge?).

```
If ray hit a vertical face (side=0):
    wall_x = camera_y + perpendicular_dist × ray_dy
If ray hit a horizontal face (side=1):
    wall_x = camera_x + perpendicular_dist × ray_dx
```

Take the fractional part (0.0 to 1.0) and scale to texture size (0 to 63):
```
tex_x = (wall_x_fractional × 64) >> 10
```

### Wall Texture Y Coordinate

For each pixel in the vertical wall strip:
```
tex_y = (pixel_y - wall_top) × 64 / wall_height
```

Pixel near the top of the wall → tex_y near 0.
Pixel near the bottom → tex_y near 63.

### Why Texture X is Computed Before Fisheye Correction

The wall hit position depends on the TRUE ray distance (before fisheye correction). Fisheye correction adjusts how tall we draw the wall, but the hit position on the wall surface doesn't change.

---

## 12. Floor & Ceiling Casting

**File:** `libs/ray.c`, inside `ray_draw_walls()`

### The Concept

For each pixel below the wall (floor) or above the wall (ceiling), we reverse-project from screen space back to world space to find which floor/ceiling tile it corresponds to.

### The Math

For a floor pixel at screen row `y`:

```
dy_from_horizon = y - screen_height/2
row_distance = (screen_height/2) / dy_from_horizon / cos_diff

world_x = camera_x + row_distance × ray_dx
world_y = camera_y + row_distance × ray_dy
```

Then extract texture coordinates from the world position's fractional part:
```
tex_x = (world_x_fractional × 64) >> 10
tex_y = (world_y_fractional × 64) >> 10
```

### Ceiling

Same math, but `dy_from_horizon = screen_height/2 - y` (measuring upward from horizon instead of downward).

### Why It Creates Perspective

Pixels near the horizon have large `row_distance` (far away floor), so the texture is compressed. Pixels near the screen bottom have small `row_distance` (nearby floor), so the texture is stretched. This naturally creates perspective foreshortening.

---

## 13. Sprite Projection (Billboard Rendering)

**File:** `libs/ray.c`, function `ray_draw_sprites()`

### What is a Billboard?

Sprites (food, power-ups, etc.) are 2D images that always face the camera. They're projected into the 3D scene but don't rotate — like cardboard cutouts that swivel to face you.

### Projection Steps

1. **Camera-space transform:** Rotate sprite position relative to camera
   ```
   tz = dx × cos(cam_angle) - dy × sin(cam_angle)   (depth)
   tx = dx × sin(cam_angle) + dy × cos(cam_angle)   (lateral offset)
   ```

2. **Screen position:** Perspective projection
   ```
   screen_x = screen_width/2 + tx × screen_width / (2 × tz)
   sprite_height = screen_height / tz
   ```

3. **Depth test:** For each vertical stripe, compare `tz` against `depth_buf[stripe]`. If the sprite is behind a wall, skip that stripe.

4. **Texture mapping:** Map screen pixel to sprite texture coordinate, skip transparent pixels.

### Painter's Algorithm

Sprites are sorted by distance (farthest first) and drawn back-to-front. This ensures closer sprites overlap farther ones correctly.

---

## 14. Procedural Texture Generation

**File:** `libs/texture.c`

### Why Procedural?

We can't load image files (no stdio, no image libraries). Instead, textures are generated mathematically at startup.

### Hash-Based Noise

```c
int tex_hash(int x, int y, int seed) {
    int h = x * 374761393 + y * 668265263 + seed;
    h = (h ^ (h >> 13)) * 1274126177;
    h = h ^ (h >> 16);
    return h;
}
```

This maps any (x, y, seed) to a pseudo-random integer. The large prime multipliers ensure even distribution. Different seeds produce different patterns.

### Texture Techniques

| Texture  | Technique                                                        |
|----------|------------------------------------------------------------------|
| Brick    | Modulo grid (16x8 bricks) + row stagger + mortar lines + noise  |
| Stone    | Larger blocks (16x16) + dark edge seams + per-block color shift  |
| Metal    | Horizontal panels + rivet dots + vertical gradient               |
| Snake    | Diamond lattice via `(x+y)%8` XOR `(x-y)%8` + highlight        |
| Floor    | Flagstone grid (21x21) + crack seams + hash noise                |
| Ceiling  | Horizontal plank pattern + seam lines + occasional knots         |
| Boundary | Diagonal stripes via `(x+y)/8 & 1` — classic warning pattern   |

### Sprite Techniques

| Sprite   | Technique                                                      |
|----------|----------------------------------------------------------------|
| Apple    | Circle test `dx²+dy² < r²` + stem + leaf + shading gradient   |
| Star     | Circle with angular radius modulation (5-pointed star shape)   |
| Skull    | Piecewise oval (head) + jaw + eye socket holes + nose bridge   |
| Bolt     | Hardcoded coordinate list for lightning shape + bright core     |

---

## 15. Lighting & Fog Model

**File:** `libs/ray.c`, function `compute_wall_light()`

### Multi-Factor Lighting

Four factors are multiplied together for the final brightness (0-256 range):

1. **Distance attenuation (quadratic):**
   ```
   norm = distance / max_distance    (0 to 256)
   attenuation = 256 - norm² / 256
   ```
   Quadratic falloff is more realistic than linear — nearby objects are bright, distant ones fade rapidly.

2. **Directional shading:**
   - E/W walls (side=1): 70% brightness (180/256)
   - N/S walls (side=0): full brightness (256/256)
   
   This simulates a light source from the north, giving walls a 3D feel.

3. **Vertical gradient:**
   ```
   vert = 256 - (pixel_in_wall × 25) / wall_height
   ```
   Slightly darker at the bottom of each wall — simulates ambient occlusion.

4. **Ambient flicker (obstacles only):**
   ```
   f = ((tick × 7 + distance) >> 3) & 15
   flicker = 242 + f
   ```
   Obstacles subtly shimmer, suggesting torchlight.

### Colored Fog

Instead of fading to black at distance, colors blend toward dark blue `(8, 10, 20)`:
```
final_color = lerp(lit_color, fog_color, fog_factor)
```

This creates atmospheric depth — distant objects have a blue haze.

---

## 16. Linear Congruential Generator (RNG)

**File:** `libs/math.c`

### The Formula

```
seed = seed × 1103515245 + 12345
```

This is a Linear Congruential Generator (LCG), the same constants used by glibc's `rand()`.

### Why Shift Right by 16?

```c
return ((seed >> 16) & 0x7FFF) % max;
```

Low-order bits of an LCG have short periods (bit 0 alternates every call). High-order bits are much more random. Shifting right by 16 extracts bits 16-30 (the good ones), masking to 15 bits (0-32767 range).

### Implementation with Bitwise Arithmetic

The LCG uses our custom `u_mul()` and `u_add()` — unsigned multiplication and addition implemented with only bitwise operations. The modulo uses `my_mod()`.

---

## 17. BFS Pathfinding for AI

**File:** `libs/ai.c`, function `bfs_toward()`

### Breadth-First Search

BFS explores all cells at distance 1, then distance 2, then distance 3, etc. This guarantees the shortest path is found first.

### Depth-Limited BFS

Our BFS stops at depth 5 (5 moves ahead). This bounds computation:
- Queue size: 128 nodes max
- Search area: ~25 cells (5² in each direction)
- Time per search: microseconds

### First-Direction Tracking

Each node in the queue remembers which direction was taken FIRST from the starting position. When we reach the target (or closest point), we return that first direction — the AI then takes one step in that direction.

```
Start → Right → Right → Down → Target
                                  ↑
        The first move was "Right", so AI goes Right this tick.
        Next tick, recalculate from new position.
```

### Manhattan Distance Fallback

If BFS doesn't reach the target within depth 5, it returns the direction that got closest (by Manhattan distance: `|dx| + |dy|`).

### Distributed Scheduling

Only 1 of 4 AI snakes runs BFS per tick:
```c
if ((ai_index & 3) == (tick & 3))  // each AI gets every 4th tick
```

This spreads the CPU cost across frames — no single frame does 4× pathfinding.

---

## 18. State Machine Pattern

**File:** `main.c`

### States

```
ST_MENU → ST_PLAYING → ST_PAUSED → ST_PLAYING
                     → ST_GAMEOVER → ST_MENU
                                   → ST_PLAYING (restart)
```

### Implementation

```c
while (state != ST_QUIT) {
    switch (state) {
    case ST_MENU:     tick_menu();     break;
    case ST_PLAYING:  tick_playing();  break;
    case ST_PAUSED:   tick_paused();   break;
    case ST_GAMEOVER: tick_gameover(); break;
    }
}
```

Each `tick_*()` function:
1. Reads input
2. Updates game state for one frame
3. Renders
4. Sleeps for frame timing
5. May change `state` to transition

### Why State Machines?

- Clear separation of concerns (menu logic vs game logic vs death animation)
- Easy to add new states
- No spaghetti of nested if/else
- Each state owns its input handling, timing, and rendering

---

## 19. Non-Blocking Input & Escape Sequences

**File:** `libs/keyboard.c`

### Raw Mode

Normal terminals buffer input until Enter is pressed (canonical mode). We disable this:

```c
new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
new_settings.c_cc[VMIN] = 0;   // don't wait for characters
new_settings.c_cc[VTIME] = 0;  // no timeout
```

Now `read()` returns immediately, even if no key was pressed.

### Non-Blocking Check via select()

```c
select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);  // tv = 0 timeout
```

`select()` checks if stdin has data available, returns immediately. If no data, the game continues without blocking.

### Arrow Key Escape Sequences

Arrow keys send 3-byte sequences:
```
Up:    \033 [ A    (ESC, [, A)
Down:  \033 [ B
Right: \033 [ C
Left:  \033 [ D
```

The parser reads byte-by-byte with a 5ms wait between ESC and the rest of the sequence to distinguish a standalone ESC keypress from an arrow key.

---

## 20. Process-Based Audio (fork + exec)

**File:** `libs/sound.c`

### One-Shot Sounds

```
Parent process                    Child process
     |                                |
     +--- fork() ----→               |
     |   (returns)        setsid()   |  (new session)
     |                    close(0,1,2)|  (detach from terminal)
     |                    execvp()   |  → afplay sound.mp3
     |                               |  (plays, then exits)
     +--- continues game
```

`fork()` creates a child process. The child replaces itself with `afplay` (macOS) or `paplay` (Linux) via `execvp()`. The parent returns immediately — audio plays asynchronously.

### Looping Music

An outer "daemon" process loops forever: fork a player child, wait for it to finish, fork another. The daemon's PID is saved so we can `kill(-pid, SIGTERM)` the entire process group when the game exits.

---

## 21. Particle System

**File:** `libs/fx.c`

### Particle Lifecycle

```
spawn → alive (life > 0) → update position → fade color → die (life = 0)
```

### Fixed-Point Position

Particles use 4-bit fractional precision (`x << 4`). This allows sub-cell movement for smooth diagonal motion.

### Fade Effect

```c
fade = particle.life * 255 / particle.max_life;
draw_r = particle.r * fade / 255;
```

A particle that starts at life=20 fades linearly to invisible over 20 frames.

### Screen Shake

`fx_shake(frames)` sets a timer. Each frame, a random offset (-1, 0, or +1) is applied to the screen origin via `scr_set_offset()`. This creates a brief jitter effect on events like collisions.

---

## 22. Snake as a Doubly-Linked List

**File:** `main.c`

### Structure

Each snake segment is an entity with `chain_next` and `chain_prev` fields, forming a doubly-linked list:

```
head ←→ seg1 ←→ seg2 ←→ ... ←→ tail
```

`head_id` points to the newest segment, `tail_id` to the oldest.

### Movement: Push Head, Pop Tail

```
Before: [H] ←→ [1] ←→ [2] ←→ [T]
Move right:
  1. Allocate new entity at (H.x+1, H.y) → becomes new head
  2. Link: [New_H] ←→ [old_H] ←→ [1] ←→ [2] ←→ [T]
  3. Free tail entity, update tail_id to [2]
After:  [New_H] ←→ [old_H] ←→ [1] ←→ [2]
```

### Growing (food eaten)

Same as movement, but skip step 3 (don't pop tail). The snake is now one segment longer.

### Shrinking (poison)

Pop 3 tails without pushing a new head. If the snake has fewer than 3 segments beyond the head, game over.

### Why Doubly-Linked?

- Push head: O(1) — allocate, link to current head
- Pop tail: O(1) — free tail, update tail to `tail.chain_prev`
- No array shifting, no copying
