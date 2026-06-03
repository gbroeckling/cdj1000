# 00 — Master Board Map

Every wire between the ESP32-S3 and the kept OEM boards in the CDJ-1000MK2, in one place.

> **Use this doc as the entry point** before reading any of the per-subsystem pages. The two visuals below show (a) where each board physically sits in the chassis and (b) the full signal/power topology.

---

## Physical layout

![Top-down chassis layout — what stays, what's gutted, where the S3 mounts](../images/00-physical-layout.svg)

> The two open-unit photos in [`/Images/`](../../Images) are the reference for this layout. The first shows the unit clamshelled open (top plate left, chassis bottom right); the second shows the chassis bottom with the **DWX2305 MAIN** board still in place. The MAIN board is gutted in v0.1 and the ESP32-S3 + a small carrier PCB sits in the same footprint, taking over every signal that used to flow from the OEM mainboard to the surrounding kept boards.

---

## Detailed wiring

![Master signal + power topology, every line labelled](../images/00-detailed-wiring.svg)

> Wires are colour-coded: red = 5 V power, blue = 3.3 V power, yellow = 12 V LED rail, dashed black = GND, blue = 3.3 V signal, orange = level-shifted (originally 5 V), magenta = LED low-side switch. Thick semi-transparent lines are multi-wire bundles.

---

## Boards we KEEP (with MK2 part references from RRV2802)

The service manual parts list confirmed:

| Board / Assembly | Drawing # | What we use it for | Notes |
|---|---|---|---|
| **JOGB Assy** (jog wheel) | **DWG1569** | Optical encoder + touch sense + vinyl LED | Same DWG as MK1 — Pioneer kept the jog mechanism |
| JOG A / JOG B / JOG C sub-parts | DNK4172 / DNK4173 / DNK4174 | sub-components of the JOGB | the encoder slotted disc + sensor + bearing carrier |
| JOG Plate | DAH2052 | aluminium platter on top | mechanical only |
| JOG Panel | DAH2182 | upper bezel ring | mechanical only |
| JOG Holder 2 | DNK4175 | mounting bracket | mechanical only |
| JOG Stay Assy | DXB1780 | jog support frame | mechanical only |
| FFC Guard | DEC2586 | ribbon cable protector | reuse with new ribbon |
| **JFLB Assy** (display board) | **DWG1568** | **LED indicators only** — VFD glass stays dark in v0.1 | µPD16306B (IC1201) not driven |
| SW Ring | DNK4065 | switch contact ring around display | with JFLB |
| **Top button PCB(s)** | (per RRV2802 schematic — confirm CN numbering) | All search / tempo / mode buttons → rewired to mux A | ~12 buttons typical |
| **Hot Cue + Loop PCB(s)** | (per RRV2802) | A/B/C + Loop In/Out + Reloop + Hot Loop + Memory + Delete → mux B | ~8 buttons; MK2 added the hot cue cluster |
| **Pitch fader assembly** | (separate part — 100 mm linear pot) | wiper → S3 ADC | **cut OEM 5 V, re-feed from 3.3 V** |
| Discrete front switches | individual tact / toggle | PLAY, CUE, VINYL/CDJ, EJECT | direct GPIO, internal pull-ups |
| OEM 12 V LED rail (CUE/PLAY/LOOP "ring" indicators) | in chassis wiring | low-side switched via IRLZ44N MOSFETs from S3 GPIO | 12 V comes from a small MT3608 boost off USB 5 V |

## Boards we REMOVE in v0.1

| Board | Drawing # | Why |
|---|---|---|
| **DWX2305 MAIN** (mainboard) | DWX2305 | replaced by ESP32-S3 + carrier PCB |
| CD drive + servo board | (DXM / DKL series) | not playing CDs |
| OEM PSU | (DWR series) | v0.1 is USB-bus only; the MT3608 boost handles the 12 V LED rail |

> **Capture-window risk reminder:** before the MAIN board comes out, logic-analyzer-capture the jog-centre position byte off the MAIN ↔ JFLB ribbon (see [`08-display.md`](./08-display.md)). Once the MAIN board is gone, that bus is dead and the v0.2 jog-centre wiring loses its decode source.

---

## Per-board breakdown — what each board sends and receives

### Jog hub PCB (part of JOGB Assy DWG1569)

The small round PCB at the centre of the jog wheel, accessed by lifting the platter (DAH2052). It carries the optical encoder, the touch sense, and the vinyl-mode indicator LED.

| Wire | Direction | S3-side destination | Carrier IC | Notes |
|---|---|---|---|---|
| VCC | →  encoder | from USB 5 V | — | 5 V required by the OEM optical encoder; 200 mA budget plenty |
| GND | bidir | star ground | — | |
| CH A | encoder →  S3 | TXS0108E A1 → B1 → **GPIO 4** | TXS0108E | hardware quadrature decode via PCNT |
| CH B | encoder →  S3 | TXS0108E A2 → B2 → **GPIO 5** | TXS0108E | direction control on PCNT |
| TOUCH | sensor →  S3 | **GPIO 6** (cap-touch input or via comparator) | (optional comp) | sensor type to verify on the MK2 — cap-sheet uses S3 native touch peripheral, pressure-sheet needs a small comparator front-end |
| VINYL LED cathode | S3 →  LED | AO3400 SOT-23 drain | small MOSFET | low-side switched from **GPIO 21**; anode tied to 3.3 V or 5 V on the OEM rail (the blue glow indicates an LED, not a VFD segment) |

Full subsystem detail: [`01-jog-encoder.md`](./01-jog-encoder.md) (jog encoder), `02-jog-touch.md` (to come), `06-leds.md` (to come, vinyl LED).

### Top button PCB

The button cluster above the jog area (TRACK SEARCH / SEARCH / TEMPO RANGE / MASTER TEMPO / TEMPO RESET / TIME MODE / AUTO CUE / TEXT MODE / JOG MODE / DIRECTION / MP3 FOLDER SEARCH ◁ ▷).

| Wire | Direction | S3-side destination | Carrier IC | Notes |
|---|---|---|---|---|
| B01..B12 | button →  mux | 74HC4067 (A) inputs 0..11 | **mux A** | one wire per momentary contact, the OEM ribbon already has them broken out for matrix scanning — we re-purpose them as 12 individual mux inputs |
| common return | bidir | mux SIG → **GPIO 12** | mux A | mux A is read by S3 GPIO 12 after the S0..S3 select bus is set to the desired channel |
| GND | bidir | star ground | — | |

Mux select bus (S0..S3) is shared between mux A and mux B, on **GPIO 8 / 9 / 10 / 11**.

### Hot Cue + Loop PCB(s)

MK2 added the Hot Cue A/B/C and Hot Loop buttons — the MK1 had none of these. Depending on board revision they may be one combined PCB or two daughter PCBs. The wiring is the same:

| Wire | Direction | S3-side destination | Carrier IC | Notes |
|---|---|---|---|---|
| B13..B20 | button →  mux | 74HC4067 (B) inputs 0..7 | **mux B** | HOT CUE A/B/C + REC MODE + LOOP IN/OUT + RELOOP + HOT LOOP + MEMORY + DELETE — exact count depends on board revision; verify against RRV2802 |
| common return | bidir | mux SIG → **GPIO 13** | mux B | |
| GND | bidir | star ground | — | |

If the final button count > 16, add a third 74HC4067 and route its SIG to GPIO 18.

### Pitch fader assembly

The 100 mm linear slider on the right side of the top plate. Three wires.

| Wire | Direction | S3-side destination | Carrier IC | Notes |
|---|---|---|---|---|
| VCC | S3 →  fader top | **3.3 V** rail (from S3 LDO) | — | **⚠ critical: cut the OEM 5 V feed to the fader and re-feed from 3.3 V**, otherwise the wiper output (~0 to 5 V) will clip and damage the S3 ADC |
| wiper | fader →  S3 | **GPIO 1 (ADC1_CH0)** | — | analog. Sample at 1 kHz, low-pass-filter in software, scale to MIDI 14-bit pitch range |
| GND | fader →  ground | star ground | — | |

Full subsystem detail: `03-pitch-fader.md` (to come).

### Discrete front switches

PLAY / PAUSE, CUE, VINYL / CDJ mode switch, EJECT.

| Switch | S3 GPIO | Notes |
|---|---|---|
| PLAY / PAUSE | **GPIO 7** | momentary; internal pull-up, active LOW |
| CUE | **GPIO 15** | momentary; internal pull-up, active LOW |
| VINYL / CDJ | **GPIO 16** | toggle / slide switch; read state, not edge |
| EJECT | **GPIO 17** | momentary; can be moved to mux if GPIO crunch happens |

### OEM 12 V LED rail (CUE / PLAY / LOOP "ring" indicators)

The large illuminated rings around the CUE and PLAY buttons (and the LOOP indicator) run on the OEM 12 V rail. With the OEM PSU removed, we generate 12 V on the carrier PCB from a small MT3608 boost off USB 5 V. Each LED string is low-side switched by an IRLZ44N MOSFET driven from the S3.

| Wire | Direction | S3-side destination | Carrier IC | Notes |
|---|---|---|---|---|
| CUE 12 V LED rail | LED →  MOSFET | IRLZ44N drain | MOSFET | gate from **GPIO 14**; source to GND |
| PLAY 12 V LED rail | LED →  MOSFET | IRLZ44N drain | MOSFET | gate from **GPIO 19** |
| LOOP 12 V LED rail | LED →  MOSFET | IRLZ44N drain | MOSFET | gate from **GPIO 20** |
| 12 V supply | boost →  LED anode | MT3608 OUT | boost | input from USB VBUS |

### JFLB plain-LED reuse (display board)

The JFLB carries the VFD glass (driven by µPD16306B at IC1201 — **not** wired in v0.1) and a handful of plain status LEDs. The blue "vinyl" indicator glow visible through the jog hub is one; mode-label LEDs (TIME / REMAIN, AUTO CUE, MEMORY, TEMPO RANGE) are the others. The exact LED count and their cathode test-points need to be confirmed at the bench when the board is open.

| Wire | Direction | S3-side destination | Carrier IC | Notes |
|---|---|---|---|---|
| LED 1 cathode | LED →  MOSFET | AO3400 drain | small MOSFET | gate from **GPIO 35**. LED anode stays on its OEM low-voltage rail (3.3 V or 5 V — verify per LED) |
| LED 2 cathode | LED →  MOSFET | AO3400 drain | small MOSFET | gate from **GPIO 36** |
| LED 3 cathode | LED →  MOSFET | AO3400 drain | small MOSFET | gate from **GPIO 37** |
| LED 4 cathode | LED →  MOSFET | AO3400 drain | small MOSFET | gate from **GPIO 38** |
| LED 5 cathode | LED →  MOSFET | AO3400 drain | small MOSFET | gate from **GPIO 39** |

The JFLB VFD glass receives no drive in v0.1. Leave the µPD16306B unpowered (or pull its strobe high to keep it idle if it shares a rail with the LEDs).

---

## Carrier PCB — the glue between OEM and S3

A small custom PCB that mounts in the footprint left by the OEM DWX2305 MAIN. It carries the support ICs that turn raw OEM signals into S3-friendly ones.

| IC | Role | Count |
|---|---|---|
| **TXS0108E** | bidir 5 V ↔ 3.3 V level translator for the jog encoder CH A / CH B | 1 (2 of 8 channels used; rest spare for v0.2 jog-centre work) |
| **74HC4067** | 16 → 1 analog/digital mux for button matrix | 2 (mux A for top buttons, mux B for hot cue / loop) |
| **IRLZ44N** | low-side N-MOSFET for 12 V CUE / PLAY / LOOP LED rails | 3 |
| **AO3400** | SOT-23 N-MOSFET for low-voltage LEDs (JFLB + vinyl) | 6 |
| **MT3608** | 5 V → 12 V boost for OEM LED rail | 1 |
| Optional comparator | jog-touch front-end if the sheet is pressure (not capacitive) | 0–1 |

Plus passives — decoupling caps on each IC's VCC, pull-ups on the discrete-button GPIOs (internal S3 pull-ups are fine; external 10 kΩ optional for noise immunity), gate pull-downs (10 kΩ) on every MOSFET so it stays off during S3 boot, and a star-ground tie point near the USB-C jack.

---

## GPIO map at a glance (v0.1 working assignment)

| GPIO | Function | Subsystem |
|---|---|---|
| 1 | pitch fader wiper (ADC1_CH0) | 03 |
| 4 | jog CH A (via TXS) | 01 |
| 5 | jog CH B (via TXS) | 01 |
| 6 | jog touch sense | 02 |
| 7 | PLAY/PAUSE button | 05 |
| 8 / 9 / 10 / 11 | mux select S0 / S1 / S2 / S3 (shared A+B) | 04 |
| 12 | mux A SIG | 04 |
| 13 | mux B SIG | 04 |
| 14 | CUE 12 V LED MOSFET gate | 06 |
| 15 | CUE button | 05 |
| 16 | VINYL / CDJ switch | 05 |
| 17 | EJECT (or move to mux) | 05 |
| 18 | spare / 3rd mux SIG if needed | 04 |
| 19 | PLAY 12 V LED MOSFET gate | 06 |
| 20 | LOOP 12 V LED MOSFET gate | 06 |
| 21 | jog VINYL LED MOSFET gate | 02 / 06 |
| 35–39 | JFLB plain-LED MOSFET gates ×5 | 06 |
| 40+ | spare (avoid 0, 45, 46 strap pins; avoid 39–42 if JTAG used) | — |

Locks once the MK2 button count is verified at the bench. The 12 / 13 / 18 assignment may shift if a 3rd mux is needed.

---

## Photo references

The two reference photos in `/Images/` are the working visual baseline for board positions:

- `normal_d4440e-cdj-1000mk2_open.jpg` — unit hinged open. Left half = top plate inverted (you're seeing the underside of the jog hub PCB and the JFLB display board). Right half = chassis bottom with CD drive bay visible.
- `normal_defc92-cdj-1000mk2_open1.jpg` — chassis bottom only, looking down at the **DWX2305 MAIN** board still in place. The square BGA centre-right is the Pioneer MCU; the Xilinx Spartan FPGA is visible upper-right area; the corner cylinders are the rubber feet.

Garry's own teardown photos (when he gets the units back from loan) will become the annotated source of truth for connector designators (`CN800`, `CN801`, etc.) — at that point the placeholders in the per-subsystem docs get replaced with concrete CN numbers from the MK2 schematic.

---

## What this doc doesn't yet contain

These open items become inputs to the per-subsystem pages as they get written:

- MK2 schematic-sheet connector designations (CN numbers) for each kept board — needs deeper service-manual reading or bench-level continuity checks
- Confirmed jog encoder PPR for MK2 (the parts list confirms DWG1569 unchanged from MK1, so 135 frames/rev is the working assumption; bench-verify when you have the unit)
- Jog touch sensor type — cap-sheet vs pressure-sheet
- Exact JFLB plain-LED identification — which silkscreen labels are LEDs vs VFD segments
- Final button count including all MK2-specific additions

Each one is tracked in the open-items section of the relevant subsystem page.
