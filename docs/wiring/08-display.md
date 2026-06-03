# 08 — Display

> **v0.1 locked plan (2026-06-03):** no big display. The OEM VFD glass stays dark. The plain LEDs on the JFLB display board (DWG1568) are reused via S3 GPIO + small MOSFETs — "better than blank" power-on feedback for ~$5 and ~1 hour of board tracing. The rich UI lives on the Traktor laptop.
>
> **v0.2+ deferred:** Path D — port djgreeb's [CDJ-1000mk3_new_life_project](https://github.com/djgreeb/CDJ-1000mk3_new_life_project) to MK2, drop in an STM32F746G-DISCO with a 4.3″ colour LCD where the VFD lived. Documented at the bottom of this file as the deferred target.

---

## v0.1 — JFLB LEDs only

### Why this scope

After comparing five paths in [`07-display-research.md`](./07-display-research.md), the v0.1 picks the cheapest reuse that needs **no HV rails**, **no protocol replay**, **no second MCU**, and **no firmware UI work**. Anything driving the VFD glass needs the OEM HV stack (−25..−35 V anode + ~3 V AC filament + µPD16306B clocking + Pioneer segment map). That's all-or-nothing — there is no partial-VFD path.

Plain LEDs on the JFLB are different: they run on 3.3 V or 5 V and have no relationship with the VFD glass. They can be driven from S3 GPIO with the same logic-level N-MOSFET technique we're already using for the OEM 12 V CUE/PLAY/LOOP rail.

### Hardware additions vs the "no display" baseline

| Part | Qty | ~Cost | Notes |
|---|---|---|---|
| Logic-level N-MOSFET (e.g. AO3400, IRLML2502) | 5 | <$1 each | low-side switch from S3 GPIO. Pick a SOT-23 part if you're hand-soldering small. |
| 10 kΩ gate pulldown | 5 | ~$0 | keeps the MOSFET off during S3 reset. |
| 220 Ω current-limit (if LED needs it) | 5 | ~$0 | only if the OEM design didn't already include it. |
| Hookup wire / Dupont | — | ~$0 | from JFLB LED pads to S3. |

Total: a few dollars. Sits inside the existing chassis power budget — no extra rail, no extra PSU.

### Sequence of operations (when the unit is open)

1. **Identify** every illuminated indicator on the JFLB:
   - VFD segments are glass-enclosed, orange-green-ish glow, addressed by µPD16306B serial bus → **skip these for v0.1.**
   - Plain LEDs are discrete surface-mount or through-hole parts, often with anode + cathode pads that trace back to a low-voltage rail + a driver transistor → **these are our targets.**
   - The **"vinyl" indicator at the centre of the jog wheel hub** is blue — VFD glass is orange-green, so a blue glow strongly implies LED. Confirm at the board level.
   - Other likely LED-driven indicators: mode labels (TIME/REMAIN, A.CUE, MEMORY, TEMPO RANGE selector) — verify per-board with continuity/scope.
2. **Tap** the cathode side of each target LED (or the gate of its existing driver transistor if present) and bring a wire out to one of the GPIOs freed up by the "no big screen" decision: **17, 18, 13, 14** and any others not yet allocated.
3. **Drive** each LED low-side via an N-MOSFET from a 3.3 V GPIO. Match the OEM rail (5 V or 3.3 V) on the LED's anode side.
4. **State machine** in firmware: map S3 internal state (USB-MIDI link up, MIDI activity, vinyl mode toggle, CUE pressed, PLAY pressed) → which LEDs are lit.

### What you get

- "**S3 is alive**" indicator on power-up (lit "vinyl" LED, for example)
- USB-MIDI link visual feedback
- Mode-indicator coverage for whichever JFLB LEDs you trace
- VFD glass numbers (BPM, time, position bar) **stay dark** — Traktor on the laptop is the source of truth for those

### What you don't get

- Rotating jog-centre position cursor — that's a VFD segment, deferred to v0.2 (but see "Jog-centre protocol-byte capture" below)
- Any BPM / time / track number on the chassis
- Any waveform / cue marker / slip-mode visualisation

---

## Pre-disassembly capture — protect the v0.2 option

Even though v0.1 doesn't wire the jog-centre, **the protocol-byte capture has to happen before the OEM mainboard comes out.** Once the mainboard is gone, the panel↔main ribbon is dead and the capture window is closed forever.

What to capture (afternoon's work with a logic analyzer):

1. Identify the panel↔main ribbon on the MK2 service-manual schematic. Find the clock, data, and any frame-sync lines.
2. Boot the OEM unit and let it run. Logic-analyze the ribbon while:
   - Platter is stopped (cursor at one position)
   - Slow rotation
   - Fast rotation
   - Eject pressed
   - CD loading
   - "Vinyl" mode toggled
   - Hot Cue A / B / C pressed individually
   - MEMORY / CALL pressed
3. Save raw captures + an annotated diff between known states.

This isolates the jog-position byte (per djgreeb's MK3 doc: 1..135 + 136 fill-circle + 137 load + 138 eject) and any MK2-specific differences from the MK3 mapping.

The output is a small `docs/protocol/mk2-bytemap.md` (to be written) that the v0.2 work picks up.

---

## v0.2+ — deferred: Path D (port djgreeb MK3 → MK2)

(Kept here as a sketch for future-Garry. Detailed plan was committed earlier; trimmed now since it's no longer the v0.1 target.)

**Concept**: replace the OEM VFD with an STM32F746G-DISCO running a port of djgreeb's firmware that simulates the CDJ-2000nxs UI (waveforms, BPM, slip mode, RGB indicators) and plays audio off SD. The S3 stays as the controller brain; the Disco is the display brain; they bridge over UART/SPI.

**Why it's a v0.2 target, not v0.1**:
- No documented MK2 port exists (djgreeb is MK3-only upstream)
- Requires the MK2 protocol delta study (Phase 1 above) — which v0.1 already covers as a byproduct of the jog-centre byte capture
- Adds a second MCU + a 4.3″ colour LCD + an SD card + bridge firmware to the BOM
- "Better than blank" plus v0.1 controller working is more valuable than month-six maybe-working CDJ-2000nxs UI

**Re-entry trigger**: v0.1 is shipped, MIDI works, the jog-centre byte capture is documented. Then revisit the djgreeb port with eyes open and a working MK2 chassis to bench against.

---

## References

- [djgreeb/CDJ-1000mk3_new_life_project](https://github.com/djgreeb/CDJ-1000mk3_new_life_project) — v0.2+ target. Repo ships the MK3 panel↔main serial protocol decode by Anatsko Andrei.
- [`07-display-research.md`](./07-display-research.md) — the path comparison that led here.
- Pioneer CDJ-1000MK2 service manual (doc RRV2802) — local copy at `docs/source/CDJ1000MK2-service-manual.pdf` (gitignored).
