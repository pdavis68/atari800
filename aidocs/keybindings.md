# Atari800 Emulator — Default Key Bindings

Derived from the source code. The primary/default platform is the SDL port
([`src/sdl/input.c`](../src/sdl/input.c)); other platform backends
(`atari_ps2.c`, `atari_x11.c`, `atari_curses.c`, `android/jni/androidinput.c`,
`javanvm/input.c`, `atari_rpi.c`) have their own, mostly hard-coded maps.

All SDL function/special keys are configurable via the `.atari800.cfg` file
(`SDL_*_KEY` options, see [`src/sdl/input.c`](../src/sdl/input.c:861)); the
values below are the compiled-in defaults ([`src/sdl/input.c`](../src/sdl/input.c:124)).

## Emulator Function Keys (SDL port defaults)

| Key | Function | Notes |
|-----|----------|-------|
| F1 | Open emulator menu (UI) | `KBD_UI` |
| F2 | Console OPTION key | `KBD_OPTION` |
| F3 | Console SELECT key | `KBD_SELECT` |
| F4 | Console START key | `KBD_START` (on 5200, returns `AKEY_5200_START`) |
| F5 | Reset | Shift+F5 = cold start, F5 = warm start ([`src/sdl/input.c`](../src/sdl/input.c:1583)) |
| F6 | Atari HELP key | `KBD_HELP` |
| F7 | Atari BREAK key | pauses BINLOAD if active ([`src/sdl/input.c`](../src/sdl/input.c:1600)) |
| F8 | Monitor menu (via UI) | `KBD_MON` → `UI_MENU_MONITOR` |
| F9 | Exit emulator | `KBD_EXIT` |
| F10 | Screenshot | Shift+F10 = interlaced screenshot ([`src/sdl/input.c`](../src/sdl/input.c:1608)) |
| F11 | On-screen keyboard | only with `USE_UI_BASIC_ONSCREEN_KEYBOARD` |
| F12 | Turbo mode toggle | `KBD_TURBO` |

## Alt-Key Combinations (SDL port, hard-coded)

Handled when Left Alt is held ([`src/sdl/input.c`](../src/sdl/input.c:1320)):

| Key | Function |
|-----|----------|
| Alt+f | Toggle full-screen/windowed |
| Alt+Shift+x | Toggle 80-column mode (XEP80/PROTO80/AF80/BIT3) |
| Alt+g | Toggle horizontal screen area |
| Alt+j | UI menu: Controller settings |
| Alt+r | UI menu: Run |
| Alt+y | UI menu: System settings |
| Alt+o | UI menu: Sound settings |
| Alt+w | UI menu: Sound recording |
| Alt+v | UI menu: Video recording |
| Alt+a | UI menu: About |
| Alt+s | UI menu: Save state |
| Alt+d | UI menu: Disk |
| Alt+l | UI menu: Load state |
| Alt+c | UI menu: Cartridge |
| Alt+t | UI menu: Cassette |
| Alt+\\ (backslash) | Black Box PBI menu (`AKEY_PBI_BB_MENU`) |
| Alt+m | Toggle mouse grab |

## Video/Colour Adjustment Keys (SDL port, hard-coded)

Only active while the emulator menu (UI) is *not* active:

| Key | Function |
|-----|----------|
| 1 / Shift+1 | Hue +/− |
| 2 / Shift+2 | Saturation +/− |
| 3 / Shift+3 | Contrast +/− |
| 4 / Shift+4 | Brightness +/− |
| 5 / Shift+5 | Gamma +/− |
| 6 / Shift+6 | Colour delay +/− |
| `[` / Shift+`[` | Scanline percentage +/− |
| 7 / Shift+7 | NTSC filter sharpness +/− |
| 8 / Shift+8 | NTSC filter resolution +/− |
| 9 / Shift+9 | NTSC filter artifacts +/− |
| 0 / Shift+0 | NTSC filter fringing +/− |
| `-` / Shift+`-` | NTSC filter bleed +/− |
| `=` / Shift+`=` | NTSC filter burst phase +/− |
| `]` | Next NTSC filter preset |

(7–`=` only when the NTSC filter is active; see [`src/sdl/input.c`](../src/sdl/input.c:1386).)

## Keyboard-Joystick Defaults (SDL port)

Configurable via `SDL_JOY_*` config options. Defaults
([`src/sdl/input.c`](../src/sdl/input.c:102)):

### Joystick 1 (kbd_layout_active[0] = on by default)

| Key | Direction |
|-----|-----------|
| Keypad 8 | Up |
| Keypad 5 | Down |
| Keypad 4 | Left |
| Keypad 6 | Right |
| Right Ctrl | Trigger/Fire |

### Joystick 2 (kbd_layout_active[1] = off by default)

| Key | Direction |
|-----|-----------|
| W | Up |
| S | Down |
| A | Left |
| D | Right |
| Left Ctrl | Trigger/Fire |

## Atari Keyboard Mapping (SDL port)

Host keys map to emulated Atari keys in [`src/sdl/input.c`](../src/sdl/input.c:1682):

| Host key | Atari key |
|----------|-----------|
| `` ` `` (backquote) | Atari key (Invert/Atari logo key) |
| Left/Right Super (Win/Cmd) | Atari key |
| Home | CLEAR (LESS when Ctrl held) |
| Pause / CapsLock | Caps toggle |
| Space | Space |
| Backspace | Backspace (DELETE) |
| Enter | Return |
| Left arrow | ← (Shift: `*`; or F3 when `F_KEYS=1`) |
| Right arrow | → (Shift: `+`... see note; or F4 when `F_KEYS=1`) |
| Up arrow | ↑ (Shift: `-`; or F1 when `F_KEYS=1`) |
| Down arrow | ↓ (Shift: `=`; or F2 when `F_KEYS=1`) |
| Escape | Escape |
| Tab | Tab |
| Delete | DELETE (Shift: DELETE LINE) |
| Insert | INSERT (Shift: INSERT LINE) |
| A–Z, a–z, digits, punctuation | Corresponding Atari keys, combined with Shift/Ctrl state |
| Ctrl+0–9 | Control-digit combinations (`AKEY_CTRL_0`…`AKEY_CTRL_9`) |
| Ctrl+letter | Control-letter combinations |

Shift and Ctrl (either side) are tracked as modifier state and XOR-ed into
every translated key ([`src/sdl/input.c`](../src/sdl/input.c:1527)).

Note on `F_KEYS` ([`src/cfg.c`](../src/cfg.c:299), default off, XL/XE machines
only): when enabled, the arrow keys emulate the Atari F1–F4 function keys
instead of cursor movement ([`src/sdl/input.c`](../src/sdl/input.c:1707)).

## Other Platforms (non-SDL, hard-coded)

### PS2 port ([`src/atari_ps2.c`](../src/atari_ps2.c:439))

Uses raw PS2 scancodes with separate switch blocks for plain, Shift, Ctrl,
and Alt modifier states; mappings are hard-coded per scancode.

### Java NVM port ([`src/javanvm/input.c`](../src/javanvm/input.c:41))

| Key | Function |
|-----|----------|
| Numpad 4/6/2/8 | Stick 0 left/right/down/up |
| Numpad 7/9/1/3 | Stick 0 diagonals |
| Left Ctrl | Stick 0 trigger |
| Tab | Stick 1 trigger |
| W/A/S/D | Stick 1 up/left/down/right |
| Q/E/Z/C | Stick 1 diagonals |

### Android port ([`src/android/jni/androidinput.c`](../src/android/jni/androidinput.c))

On-screen touch controls; keyboard events come through a ring buffer of
Atari key codes (`Android_Keyboard`), no host key bindings.

### Raspberry Pi port ([`src/atari_rpi.c`](../src/atari_rpi.c))

Uses its own minimal keyboard handling; see source for details.

## Config File Options for Rebinding (SDL port)

All of the following accept an SDL key number in `.atari800.cfg`
([`src/sdl/input.c`](../src/sdl/input.c:757), [`src/sdl/input.c`](../src/sdl/input.c:861)):

```
SDL_JOY_0_LEFT / RIGHT / UP / DOWN / TRIGGER
SDL_JOY_1_LEFT / RIGHT / UP / DOWN / TRIGGER
SDL_UI_KEY / SDL_OPTION_KEY / SDL_SELECT_KEY / SDL_START_KEY
SDL_RESET_KEY / SDL_HELP_KEY / SDL_BREAK_KEY / SDL_MON_KEY
SDL_EXIT_KEY / SDL_SSHOT_KEY / SDL_ONSCREEN_KEY / SDL_TURBO_KEY
```
