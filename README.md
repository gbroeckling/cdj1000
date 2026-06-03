# cdj1000

### Pioneer CDJ-1000MK2 → ESP32-S3 USB-MIDI controller for Traktor Pro 4

Resurrect a Pioneer CDJ-1000MK2 (2003, no native USB/MIDI) as a class-compliant USB-MIDI / HID controller for Traktor Pro 4. The OEM mainboard and CD drive are removed; the chassis, jog wheel assembly, pitch fader, button PCBs (including the MK2-added Hot Cue A/B/C and Hot Loop buttons), pots, and encoders are kept and re-wired to an **ESP32-S3 DevKitC-1 (N16R8)** that speaks USB-MIDI natively via TinyUSB.

> **Status:** architecture v0.2 — target unit confirmed as CDJ-1000**MK2** (service manual doc RRV2802). Wiring topology unchanged from v0.1; button count and display driver IC pending MK2 verification.

---

## Why ESP32-S3 (not Teensy)

Prior community builds (Lee Smith / DJLegionUK) used Teensy 3.6. The S3 wins on:

- Native USB-OTG → class-compliant USB-MIDI via TinyUSB, no shield/adapter
- More GPIO + 2× ADC blocks, PCNT peripheral for jog quadrature, RMT for WS2812
- 8 MB PSRAM → enough framebuffer headroom for an optional jog-center IPS display
- WiFi/BLE for future Ableton Link / OSC / wireless config
- Cheaper, current production silicon

---

## Hardware re-use map

| Kept | Replaced |
|---|---|
| Chassis, top plate, jog wheel assembly | Pioneer mainboard |
| Jog optical encoder (~135 frames/rev — verify MK2) | CD drive + servo board |
| Jog touch sheet | OEM display board |
| 100 mm pitch fader | OEM VFD (optional re-use — see [`07-display-research.md`](docs/wiring/07-display-research.md)) |
| Button PCBs — search, tempo, master tempo, time, direction, memory, delete, jog mode, vinyl/CDJ + MK2 Hot Cue A/B/C + Hot Loop | Power supply (USB-powered now) |
| Pots & rotary encoders | |
| CUE / PLAY / LOOP LEDs (12 V — driven via MOSFETs) | |

---

## Wiring overview

Detailed per-subsystem wiring docs live in [`docs/wiring/`](docs/wiring/). Quick reference:

| Subsystem | OEM source | ESP32-S3 GPIO | Notes |
|---|---|---|---|
| Jog rotation | DEC2498 encoder plate (CH A/B) | GPIO 4 / 5 (PCNT) | 5 V → 3.3 V level shift (TXS0108E) |
| Jog touch | DSX1060 sheet sense | GPIO 6 | Verify cap vs pressure |
| Pitch fader | 100 mm linear pot wiper | GPIO 1 (ADC1_CH0) | Cut OEM 5 V; re-feed slider from 3.3 V |
| Buttons (~20 incl. MK2 hot cue A/B/C + hot loop) | Button PCBs | 2× 74HC4067, S0–S3 = GPIO 8/9/10/11, SIG = GPIO 12/+1 — may need 3rd mux on MK2 |
| PLAY / PAUSE | discrete | GPIO 7 | |
| CUE | discrete | GPIO 15 | |
| VINYL/CDJ switch | discrete | GPIO 16 | |
| OEM CUE/PLAY/LOOP LEDs | 12 V rail | IRLZ44N from GPIO | Lee Smith MK1 gotcha |
| Status / pad RGB | new | WS2812 ← GPIO 13 | |
| Display (optional) | new | GC9A01 SPI or SSD1306 I2C | jog-center vs status |
| Power | USB 5 V → VBUS | 3.3 V LDO; MT3608 boost for 12 V LEDs only | Star ground |

Full GPIO table and wiring SVGs: [`docs/wiring/README.md`](docs/wiring/README.md).

---

## Reference builds

| Project | Why it matters |
|---|---|
| [spectran/CDJ-100S-MIDI-Adapter](https://github.com/spectran/CDJ-100S-MIDI-Adapter) | Closest prior art. STM32F103 replacing the mainboard on a CDJ-100S; full schematic + `Connection_scheme.pdf` + VirtualDJ XML. Source of the 3.3 V pitch-fader fix. |
| [pestrela/dj_maps](https://github.com/pestrela/dj_maps) | Richest Traktor Pro mappings — including DDJ-1000 with BOME jog-screen feedback. Pattern source for our `.tsi` map. |
| Lee Smith / DJLegionUK — CDJ-1000 Teensy builds | Original MK1 article [djtechtools.com 2017](https://djtechtools.com/2017/06/28/hacking-cdj-1000mk1-work-midi-controller-traktor-scratch/) + later drop-in PCBs covering MK1/**MK2**/MK3. |
| MK3 conversion reference | [Converting A Dead CDJ-1000MK3 To A MIDI Controller — DJ TechTools](https://djtechtools.com/2017/07/28/converting-dead-cdj-1000mk3-midi-controller/) |
| Pioneer service manual (MK2) | Doc **RRV2802** — available via [ManualsLib](https://www.manualslib.com/manual/1059617/Pioneer-Cdj-1000mk2.html), elektrotanya. |

Local clones of the first two repos live under `references/` (gitignored).

---

## Open items (MK2)

1. Confirm jog encoder voltage + PPR from the **MK2** service manual (RRV2802)
2. Jog-touch sensor type on MK2 — capacitive vs pressure sheet (determines S3 native touch vs comparator front-end)
3. Trace MK2 button PCB connector (CN) pinout → 4067 channel map. Include Hot Cue A/B/C + Hot Loop in the count.
4. Lock final GPIO map after button count — MK2 may need a 3rd 4067 mux
5. Identify the MK2 VFD driver IC (MK1 used NEC µPD16306B; MK2 service manual lists buffer/inverter chips around it but the actual driver part needs verification)
6. Display decision — Path A (reuse OEM VFD via protocol replay) **before** mainboard removal, or Path C (replace with modern OLED/IPS). See [`docs/wiring/07-display-research.md`](docs/wiring/07-display-research.md).
7. Firmware path — ESP-IDF + TinyUSB MIDI **vs** Arduino-ESP32 + Adafruit TinyUSB

---

## Layout

```
.
├── docs/
│   ├── wiring/         # per-subsystem wiring diagrams + signal tables
│   ├── images/         # photographs, annotated PCB shots, SVG schematics
│   └── source/         # service manual PDF + other source docs (gitignored)
├── firmware/           # ESP-IDF or Arduino sketch (TBD)
├── hardware/           # BOM, KiCad project (TBD)
├── mappings/           # Traktor Pro 4 .tsi + BOME files
└── references/         # cloned reference repos (gitignored)
```

---

## License

GPL-3.0 — see [LICENSE](LICENSE). Same license as [padspanHA](https://github.com/gbroeckling/padspanHA).
