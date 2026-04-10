# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Upload

This is an Arduino sketch — there is no Makefile. Build and upload using the **Arduino IDE** (no `arduino-cli` is installed). Open `starfield-led-prop.ino` in the IDE and use the standard Verify/Upload buttons.

**Required libraries** (install via Arduino Library Manager):
- `MIDI Library` (FortySevenEffects)
- `Adafruit NeoPXL8`

**Target board:** Adafruit Feather M4 Express (SAMD51)

## Hardware

- **MCU:** Adafruit Feather M4 with NeoPXL8 M4 FeatherWing
- **Display:** 80×56 LED grid = 4,480 NeoPixels driven across 8 parallel data pins
  - Each pin drives 896 pixels (one vertical strip of 16 logical columns × 56 rows each)
  - Pins: `{ 13, 12, 11, 10, SCK, 5, 9, 6 }`
- **MIDI input:** Hardware MIDI via `Serial1` at 31250 baud (standard MIDI rate)

## Architecture

The entire sketch is a single `.ino` file. Its structure:

1. **PROGMEM bitmaps** — Greyscale 80×56 images stored as bit-packed byte arrays in flash. Each image is 80 columns × 7 bytes/column (56 bits). Images are converted from PNG/XCF using a local fork of `image2cpp` at `file:///Users/mbiwinds/Documents/Arduino/image2cpp/index.html` with settings: Background=Black, Brightness threshold=10, Rotate=90°, Draw Mode=Horizontal, Swap Bits=YES, ZigZag=YES.

2. **Global animation state** — A single `uint8_t animation` variable selects the active animation. Each animation has its own `*StartTime` and `*Initialized` pair for one-shot reset logic.

3. **MIDI handler** (`handleNoteOn`) — Maps incoming MIDI notes to animation IDs. Reset `*Initialized = false` before changing `animation` so the new animation restarts cleanly.

4. **`animate()` dispatcher** — Called every `loop()` iteration; switches on `animation` and calls the appropriate `animate*()` function.

5. **`loop()`** — Calls `MIDI.read()` then `animate()` on every iteration with no blocking delays.

## MIDI Note → Animation Map

| Note   | Animation ID | Name           |
|--------|-------------|----------------|
| C0     | 0           | Setup (standby)|
| D0     | 1           | Blackout       |
| E0     | 2           | Stars (twinkle)|
| F0     | 3           | These (pulse)  |
| G0     | 4           | Collapse       |
| A0     | 5           | Explode        |
| B0     | 6           | Scatter        |
| C1     | 7           | Constellation  |
| D1     | 8           | Rising         |
| E1     | 9           | Dipper         |
| F1     | 10          | Aurora         |
| A1     | 13          | Rain           |
| B1     | 14          | Plasma         |

## Pixel Coordinate System

`getPixelIndex(col, row)` converts visual (col, row) to the linear NeoPixel index, accounting for zigzag wiring:
- **Even columns:** pixel 0 = top (row 0), pixel 55 = bottom (row 55) — normal order
- **Odd columns:** pixel 0 = bottom (row 55), pixel 55 = top (row 0) — reversed

When iterating bitmap data for direction-sensitive animations (e.g., drawing a line bottom-to-top), odd and even columns must be iterated in opposite row directions to maintain visual consistency.

## Adding a New Animation

1. Add `uint32_t fooStartTime = 0; bool fooInitialized = false;` globals.
2. Write `void animateFoo()` following the `if (!fooInitialized) { ... }` init pattern.
3. Add a `case NOTE_X0:` in `handleNoteOn` that sets `fooInitialized = false` and `animation = N`.
4. Add `case N: animateFoo(); break;` in `animate()`.
5. If the animation uses a bitmap, add the PROGMEM array (converted via image2cpp) before `handleNoteOn`.
