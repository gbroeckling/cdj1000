# 07 — Display: OEM VFD reuse research

> **Question:** can we keep the CDJ-1000**MK2**'s original VFD and drive it from the ESP32-S3?
> **Short answer:** technically yes, but no one's done it publicly for either MK1 or MK2. Every documented build removed the VFD. The engineering effort is several weekends; the aesthetic delta vs a modern OLED is modest.
>
> **Note:** the protocol/driver-IC notes below were originally researched for the MK1 (NEC µPD16306B on JFLB ASSY DWG1549). The MK2 service manual (doc RRV2802) lists buffer/inverter chips around the VFD (TC74VHC541FT, TC7S04FU, TC7WT241FU, TC7WU04FU) but the actual VFD driver IC part number on MK2 still needs to be read off the MK2 schematic. The MK1/MK2 conclusions are otherwise the same — see "Recommendation" below.

---

## The OEM display board

- **MK1 assembly:** JFLB ASSY, drawing DWG1549 (per service manual)
- **MK1 driver IC:** NEC **µPD16306B** — standard VFD driver/controller, serial bit-banged interface (clock / data / strobe / blanking). Datasheets exist but are not on clean public mirrors.
- **MK2 display board (doc RRV2802):** uses different silicon. Buffer/inverter chips identified — TC74VHC541FT (IC405), TC7S04FU (IC407), TC7WT241FU (IC605), TC7WU04FU (IC402). The driver IC itself is **not** µPD16306B and the actual part number still needs to be read off the MK2 schematic.
- **Glass:** custom Pioneer segments — TIME, BPM, position bar, MEMORY / CALL slots, track number. Fixed glyph vocabulary; no arbitrary pixels.
- **Power needed by the VFD glass:**
  - Anode/grid: **−25 V to −35 V DC**
  - Filament heater: **~3 V AC**, typically a 1–2 kHz square wave
  - Stock origin: rails come off the AC-mains PSU. Once the OEM PSU is removed for a USB-bus build, those rails disappear.

---

## What every successful builder has done

| Build | OEM display handling |
|---|---|
| **Andrei Anatska — CDJ-1000 → CDJ-2000 UI** (the high-water mark) | Removed VFD. Soldered wires from mainboard to an STM32F746G Discovery board's built-in colour LCD. Replicated the CDJ-2000 UI in software. |
| **Lee Smith / DJLegionUK — MK1 Teensy MIDI** | Removed VFD. No replacement. Status comes from Traktor. |
| **spectran — CDJ-100S STM32** | Removed VFD. No replacement. |
| **AlejandroPerez92 — CDJ-800 MK2** | Decoded the MK2 SPI protocol ("like CDJ-1000 MK3 but with fewer bytes"). Working code reads jog/pitch/buttons and drives the display. But the protocol is **MK2/MK3** — MK1 is older and not publicly decoded. |

The closest piece of public reverse-engineering work targets the MK3 (Scribd doc). The MK1's panel↔main serial link is simpler and **not** publicly documented.

---

## Can the ESP32-S3 drive it?

Yes — three paths, ranked.

### Path A — black-box the display board, replay the panel↔main protocol  *(most realistic)*

Tap the ribbon cable between Main Assembly and Display Board **while the OEM mainboard is still alive**. Capture with a logic analyzer (Saleae or cheap clone) at known states — known track, known time, known BPM, paused vs playing. Decode the byte format. Then replay those frames from the S3 over the same ribbon. The display board's own MPU keeps driving the µPD16306B; you only need to look like the OEM main.

**Pros:** highest chance of working. The S3 has plenty of clock headroom — the Arduino-forum CDJ-800 thread found old Arduino too slow but ESP8266 fine; S3 is well above that bar.

**Cons:**
- Window closes once you gut the mainboard — you cannot capture the protocol afterwards.
- Still need to keep the HV rails for the VFD alive. Either keep the OEM PSU board in and run it from mains (only use USB for data), or design a small HV inverter from 5 V USB.
- You're stuck with Pioneer's segment vocabulary.

### Path B — drive the µPD16306B directly, skip the panel MPU

µPD16306B datasheet exists; commands are documented. But **Pioneer's segment map** (which bit lights which segment of the custom glass) is not publicly documented. You'd be reverse-engineering both the IC config and the segment mapping.

**Pros:** simpler topology — S3 → µPD16306B over serial.
**Cons:** more reverse engineering than A. Still need the HV rails.

### Path C — replace the VFD with a modern display  *(strongly recommended)*

Drop a small modern panel in the VFD window cutout:

- **SSD1306 0.96″/1.3″ OLED, I²C** — 128×64, white or yellow elements. With an orange acrylic filter or a yellow-element variant, very close to VFD aesthetic. ~20 mA from 3.3 V. Same info density as the OEM VFD plus arbitrary layout.
- **ST7789 / GC9A01 IPS, SPI** — 1.5–2.1″, full colour. Lets you show a live position bar, BPM, cue markers, anything. Higher current, but trivial on USB 5 V.

This is what every successful build has done.

---

## Decision history

**2026-06-03, v0.1 lock (current):** **No big display in v0.1.** Reuse the JFLB plain-LED indicators only (vinyl indicator + 1–3 mode LEDs). VFD glass stays dark. See [`08-display.md`](./08-display.md). Jog-centre protocol byte gets captured during disassembly to keep the v0.2+ option alive.

**2026-06-03, earlier in the day:** Path D selected — port [djgreeb/CDJ-1000mk3_new_life_project](https://github.com/djgreeb/CDJ-1000mk3_new_life_project) from MK3 to MK2. **Deferred to v0.2+** after scope reduction; the published firmware and protocol decode still make it the right target if/when the big-screen ambition returns.

(Old recommendation kept below for historical context.)

---

## ~~Old recommendation~~ (superseded by Path D)

~~**Path C unless you commit to Path A *before* removing the mainboard.**~~

The aesthetic loss vs an OEM-looking VFD is real but small — a well-styled OLED in the same window with a retro orange palette captures ~90% of the feel. The engineering delta — HV inverter plus logic-analyzer protocol capture (A) or full driver reverse-engineering (B) — is several weekends of risky work, and the end result is still constrained to Pioneer's segment vocabulary.

If you genuinely want Path A:

1. Get a working CDJ-1000 MK1 (or borrow ribbon access on a friend's) and a logic analyzer.
2. Capture display-bus frames at a matrix of states: stopped/playing/cueing × known BPMs × known time codes × MEMORY/CALL.
3. Decode the byte format into a frame schema (probably small — a few dozen bytes per frame).
4. Build a small S3 replayer (ESP-IDF, simple serial bit-bang).
5. Keep the OEM PSU board in the chassis — pull mains to it, take 5 V from USB for data, and let the OEM PSU supply the VFD HV rails. Do **not** try to invert HV from USB.
6. Verify on the bench against the captured frames before final assembly.

---

## Open follow-ups

(All resolved — see [`08-display.md`](./08-display.md) for the locked v0.1 plan.)

---

## Sources

- [Why Buy The Newer Model, When You Can Just Replicate Its User Interface? — Hackaday](https://hackaday.com/2020/08/08/why-buy-the-newer-model-when-you-can-just-replicate-its-user-interface/)
- [Hacking A CDJ-1000MK1 To Work As A MIDI Controller in Traktor Scratch — DJ TechTools](https://djtechtools.com/2017/06/28/hacking-cdj-1000mk1-work-midi-controller-traktor-scratch/)
- [Converting A Dead CDJ-1000MK3 To A MIDI Controller — DJ TechTools](https://djtechtools.com/2017/07/28/converting-dead-cdj-1000mk3-midi-controller/)
- [Reverse Engineering Pioneer CDJ-1000 mk3 — YouTube](https://www.youtube.com/watch?v=yRItR1R8qdQ)
- [Hacking CDJ-800 Display Communication Protocol — Arduino Forum p3](https://forum.arduino.cc/t/hacking-cdj800-display-communication-protocol/1133083?page=3)
- [Pioneer CDJ-1000 Service Manual — ManualsLib](https://www.manualslib.com/manual/1198440/Pioneer-Cdj-1000.html)
- [Reverse Engineering Pioneer CDJ-1000 Serial Protocol — Scribd](https://www.scribd.com/document/552171298/Reverse-Engineering-Pioneer-CDJ-1000-Serial-Protocol)
- [CDJ-1000 MK3 for Rekordbox — vandoeselaar tinkering log](https://www.vandoeselaar.com/tinkering/cdj-1000-mk3-for-rekordbox/)
