# 08 — Display: Path D (port djgreeb MK3 → MK2)

> **Decision (2026-06-03):** the OEM VFD is removed. The display window is reused as the mounting cutout for an **STM32F746G-DISCO** board running a port of djgreeb's [CDJ-1000mk3_new_life_project](https://github.com/djgreeb/CDJ-1000mk3_new_life_project), which simulates the CDJ-2000nxs UI (waveforms, BPM counter, slip-mode, RGB indicators) and plays audio from SD.
>
> Background: see [`07-display-research.md`](./07-display-research.md).

---

## Why Path D over the alternatives

| Path | Outcome | Verdict |
|---|---|---|
| A — replay OEM panel↔main protocol → drive OEM VFD | Original look, segment vocabulary locked to Pioneer's; needs HV rails kept alive and protocol captured *before* mainboard removal | Rejected: high effort, low payoff |
| B — drive µPD16306B directly | Same look as A; segment map not public | Rejected: more reverse engineering than A |
| C — drop-in SSD1306/GC9A01 in VFD cutout | Clean, simple, modern UI we write ourselves | Rejected: we'd be reinventing UI/audio/SD work that already exists |
| **D — port djgreeb MK3 → MK2** | CDJ-2000nxs-style colour UI, RGB waveform, slip mode, SD playback — all already written | **Chosen** |

The deciding factor was discovering djgreeb's repo ships **`Reverse Engineering Pioneer CDJ-1000 serial protocol.pdf`** (by Anatsko Andrei) — a byte-by-byte decode of the MK3 Display Board ↔ Main Assy serial bus. djgreeb's firmware speaks that protocol. To port to MK2 we re-target the protocol decoder; we don't have to redo the UI, audio, SD, or display driver work.

---

## What "port" actually means

We keep two things of djgreeb's:

1. **The hardware bracket** — STM32F746G-DISCO board (the "Disco board") mounted where the OEM VFD lived. Its built-in 4.3″ capacitive-touch colour LCD becomes the new display.
2. **The firmware stack** — UI rendering, SD-card audio, waveform overlay, RGB indicator state machine. Distributed as prebuilt `.hex` files (`CDJ1000VER086B.hex` → `CDJ1000VER117.hex`) plus a source archive (`CDJ1000_new_life_project.rar`) in the repo.

We **replace** one thing:

3. **The input/protocol layer** — djgreeb consumes the **MK3** panel↔main serial bytes. We need to decode the **MK2** equivalent and adapt the firmware's input parser. Per the CDJ-800 MK2 analogue ("like CDJ-1000 MK3 but with fewer bytes"), the structure should be similar with some MK3-only fields (e.g., the 8-package waveform format) absent or shorter on MK2.

This split is what makes Path D realistic. The hardest parts of building a CDJ-2000nxs-style UI from scratch (graphics, audio decode, file system, display driver) are *done*. We pick up the work right at the protocol seam.

---

## Hardware bill of materials (display subsystem only)

| Part | Source | ~Cost USD | Notes |
|---|---|---|---|
| STM32F746G-DISCO development board | ST / Mouser / DigiKey | $50–55 | Has the 4.3″ 480×272 capacitive-touch LCD onboard. This is the *display brain* — separate from the ESP32-S3 *controller brain*. |
| micro-SD card (Class 10, ≤ 32 GB FAT32) | any | $5–10 | Audio source for the SD playback mode. |
| Custom mounting bracket | 3D print | — | djgreeb's repo includes 3D files (`Encoder knob 3D/`) and an assembly manual PDF. |
| Ribbon harness, OEM Pioneer → Disco GPIO | hand-built | $5 | Replaces the OEM mainboard↔display-board connection. |

The ESP32-S3 (controller brain) and STM32 Disco (display brain) communicate over UART or SPI — exact link TBD when we lay out the architecture for both halves.

---

## Architecture split

```
                ┌───────────────────────────────────────────┐
                │  ESP32-S3 DevKitC-1 (controller brain)    │
                │  • USB-MIDI to host (TinyUSB)             │
                │  • jog PCNT, fader ADC, button mux        │
                │  • discrete buttons + LED MOSFETs         │
                │  • WS2812 status                          │
                └───────────────────────┬───────────────────┘
                                        │ UART/SPI bridge
                                        ▼
                ┌───────────────────────────────────────────┐
                │  STM32F746G-DISCO  (display brain)        │
                │  • CDJ-2000nxs UI (djgreeb firmware port) │
                │  • SD-card audio playback                 │
                │  • Waveform / BPM / cue render            │
                │  • 4.3″ capacitive-touch LCD              │
                └───────────────────────────────────────────┘
```

Each half stays independent. The S3 doesn't know how the screen renders; the STM32 doesn't know how MIDI bytes get to the host. The bridge between them carries: track state, BPM, jog position, button-press events, cue-point bookkeeping.

---

## Porting plan (MK3 → MK2)

### Phase 1 — protocol delta study (no soldering)

1. Read djgreeb's `Reverse Engineering Pioneer CDJ-1000 serial protocol.pdf` and the firmware source (`CDJ1000_new_life_project.rar`) end-to-end to fully understand the MK3 input parser.
2. Pull the **MK2 panel↔main schematic** from the MK2 service manual (doc RRV2802) — identify the equivalent ribbon. Locate signal names: clock, data, sync, frame markers.
3. With the MK2 unit still functional, **logic-analyzer-capture** the MK2 ribbon at known states (paused / playing / cueing / loading × known BPM × known time × MEMORY recall × hot cue press).
4. Diff captured byte streams against the documented MK3 byte mapping. Build a delta document.

### Phase 2 — firmware patch

5. In djgreeb's source, locate the input-parser entry point.
6. Branch a `cdj1000mk2` fork; replace the MK3 byte map with the MK2 byte map.
7. Test against a re-feed of captured logic-analyzer dumps before connecting real hardware.

### Phase 3 — hardware integration

8. Remove OEM mainboard + CD drive + display board.
9. Mount STM32 Disco where the VFD was; route the OEM panel↔main ribbon (or its replacement) into Disco GPIOs per djgreeb's pinout, **plus** the MK2-specific signals identified in Phase 1.
10. Bring up the Disco firmware. Confirm UI renders, SD playback works, jog/buttons feed through correctly.

### Phase 4 — link to ESP32-S3

11. Define the UART/SPI bridge between the S3 (controller) and the Disco (display) for shared state (BPM, track position, cue markers).
12. Confirm Traktor sees correct MIDI/HID output from the S3 while the Disco shows correct state on the LCD.

---

## Open questions

- [ ] Is djgreeb's source license permissive enough for us to redistribute a fork? (Repo doesn't carry an obvious LICENSE file at the time of cloning — check with djgreeb directly before publishing a fork.)
- [ ] Does the MK2 panel↔main bus run at the same clock rate as MK3, or slower?
- [ ] Does the MK2 expose fewer "packages" than MK3 (the CDJ-800 MK2 analogue is "MK3 minus bytes")?
- [ ] Is there a Hot Cue A/B/C bit in the MK3 protocol decode that needs renaming for MK2 (which calls them the same), or are the MK2 hot cues mapped to entirely different bytes?

---

## Critical risks

1. **Capture-window risk.** The logic-analyzer capture in Phase 1 step 3 requires a **working** MK2. Once the mainboard is removed in Phase 3, you cannot go back and capture more states. Capture extensively before disassembly — paused, playing, all hot cues pressed individually, all memory recalls, loop in/out, eject, every status LED state.
2. **License risk.** djgreeb's repo has no LICENSE file visible. Forking and redistributing without permission is shaky. Reach out before publishing a port; worst case, distribute only the MK2-delta patch and instructions for users to apply against an upstream-checked-out tree.
3. **Disco availability.** The STM32F746G-DISCO is still produced but lead times fluctuate. Order before committing to the build.

---

## References

- [djgreeb/CDJ-1000mk3_new_life_project](https://github.com/djgreeb/CDJ-1000mk3_new_life_project) — firmware, hex builds, assembly manual, protocol PDF.
- [DIY: Upgrade A 12 Year Old CDJ-1000MK3 — DJ TechTools 2018](https://djtechtools.com/2018/05/21/diy-upgrade-a-12-year-old-cdj-1000mk3-sd-card-rgb-waveform-slip-mode/)
- [CDJ-1000mk3 Gets Amazing NXS-Style Display Mod — DJ TechTools 2017](https://djtechtools.com/2017/06/20/cdj-1000mk3-gets-amazing-nxs-style-display-mod/)
- [Why Buy The Newer Model — Hackaday 2020 (Anatska CDJ-1000 → CDJ-2000)](https://hackaday.com/2020/08/08/why-buy-the-newer-model-when-you-can-just-replicate-its-user-interface/)
- Pioneer CDJ-1000MK2 service manual (doc RRV2802) — local copy at `docs/source/CDJ1000MK2-service-manual.pdf` (gitignored).
