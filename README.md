<div align="center">

```
    ___       ___       ___       ___       ___       ___       ___       ___       ___   
   /\  \     /\  \     /\  \     /\  \     /\  \     /\  \     /\  \     /\  \     /\  \  
  /::\  \   _\:\  \   /::\  \   /::\  \   /::\  \   /::\  \   /::\  \   /::\  \   _\:\  \ 
 /\:\:\__\ /\/::\__\ /:/\:\__\ /:/\:\__\ /:/\:\__\ /\:\:\__\ /::\:\__\ /:/\:\__\ /\/::\__\
 \:\:\/__/ \::/\/__/ \:\:\/__/ \:\:\/__/ \:\:\/__/ \:\:\/__/ \:\:\/  / \:\/:/  / \::/\/__/
  \::/  /   \:\__\    \::/  /   \::/  /   \::/  /   \::/  /   \:\/  /   \::/  /   \:\__\  
   \/__/     \/__/     \/__/     \/__/     \/__/     \/__/     \/__/     \/__/     \/__/  
```

<br>

<img src="https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=white" />
<img src="https://img.shields.io/badge/Platform-Terminal-black?style=for-the-badge&logo=windowsterminal&logoColor=white" />
<img src="https://img.shields.io/badge/stdlib-ZERO-ff3333?style=for-the-badge" />
<img src="https://img.shields.io/badge/Vibes-Nokia_3310-2FC446?style=for-the-badge" />

<br><br>

```
╔══════════════════════════════════════════╗
║  ┌────────────────────────────────────┐  ║
║  │                                    │  ║
║  │         oooooo@         *          │  ║
║  │                                    │  ║
║  │                                    │  ║
║  └────────────────────────────────────┘  ║
║           S C O R E :  0 4 2             ║
║                                          ║
║    ┌───┐                                 ║
║    │ W │        ╔═══════════════╗        ║
║  ┌─┼───┼─┬───┐ ║  N O K I A    ║        ║
║  │ A │ S │ D │ ║  S N A K E    ║        ║
║  └───┴───┴───┘ ╚═══════════════╝        ║
╚══════════════════════════════════════════╝
```

<h3>A classic Snake game built <i>entirely from scratch</i> in C</h3>
<p>No <code>printf</code>. No <code>malloc</code>. No <code>strlen</code>. Just raw system calls.</p>

---

</div>

## 🐍 The Wiggle

```
  Frame 1:       Frame 2:       Frame 3:       Frame 4:

  oooo@          .oooo@         ..oooo@        ...oooo@
                                                      *  nom!

  Frame 5:       Frame 6:       Frame 7:       Frame 8:

  ....ooooo@     .....ooooo     ......oooo     .......ooo
                          @│          o@│           oo@│
                           │            │     (growing!)│
```

---

## 🔧 Five Custom Libraries — Zero Dependencies

> _"We don't use libraries. We **build** them."_

```
  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
  │  memory.c   │   │  string.c   │   │   math.c    │
  │─────────────│   │─────────────│   │─────────────│
  │ mmap()      │   │ str_len()   │   │ seed_rng()  │
  │ munmap()    │   │ int_to_str()│   │ rng_range() │
  │             │   │             │   │ in_bounds() │
  │ replaces:   │   │ replaces:   │   │ hits_body() │
  │ malloc/free │   │ strlen/sprintf  │ replaces:   │
  └─────────────┘   └─────────────┘   │ rand/math.h │
                                       └─────────────┘
  ┌─────────────┐   ┌─────────────┐
  │  screen.c   │   │ keyboard.c  │
  │─────────────│   │─────────────│
  │ scr_clear() │   │ kb_raw()    │
  │ scr_goto()  │   │ kb_restore()│
  │ scr_putch() │   │ kb_read()   │
  │ scr_puts()  │   │             │
  │ draw_box()  │   │ replaces:   │
  │ replaces:   │   │ getchar/    │
  │ printf/     │   │ scanf       │
  │ ncurses     │   └─────────────┘
  └─────────────┘

        All rendering via write() syscall
        All input via select() + read()
```

---

## 🕹️ Play

```bash
make        # compile
./snake     # play!
make clean  # remove binary
```

## 🎮 Controls

```
         ┌───┐
         │ W │  ↑
     ┌───┼───┼───┐
     │ A │ S │ D │
     └───┴───┴───┘
       ←   ↓   →

     Q = Quit
```

## 📏 Rules

```
  ╔═══════════════════════════════════════╗
  ║                                       ║
  ║  @ = your snake's head                ║
  ║  o = your snake's body                ║
  ║  * = food (eat to grow!)              ║
  ║  # = walls (touch = game over)        ║
  ║                                       ║
  ║  Don't eat yourself. That's weird.    ║
  ║                                       ║
  ╚═══════════════════════════════════════╝
```

---

## 📂 Project Structure

```
wiggleboi/
│
├── main.c ·············· game loop and core logic
├── Makefile ············· build configuration
│
└── libs/
    ├── memory.h / .c ··· mmap/munmap allocator
    ├── string.h / .c ··· str_len, int_to_str
    ├── math.h   / .c ··· RNG, bounds, collision
    ├── screen.h / .c ··· ANSI terminal rendering
    └── keyboard.h / .c · raw mode input handling
```

---

## ⚡ Technical Highlights

| Feature | Detail |
|---------|--------|
| 🚫 Standard Library | Zero `stdio.h` / `stdlib.h` / `string.h` usage |
| 🧠 Custom Allocator | `mmap()` — requests memory directly from the OS kernel |
| 🎨 Rendering | Differential — only 2-3 `write()` calls per frame |
| 👻 Flicker-free | Cursor hidden during gameplay |
| 🎲 True Randomness | Seeded from `time()` — every game is different |
| 📐 Code Size | ~285 lines of actual code |

---

<div align="center">

```
        ┌──────────────────────────────────────┐
        │                                      │
        │   Built with raw C and zero chill.   │
        │                                      │
        │          ~ WiggleBoi 2025 ~          │
        │                                      │
        └──────────────────────────────────────┘

                ooooooooooooo@
```

</div>
