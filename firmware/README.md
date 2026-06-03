# Firmware — ESPHome on ESP32-S3 (esp-idf framework)

> The v0.1 firmware uses **ESPHome** so the wiring spec in [`../docs/wiring/`](../docs/wiring/) maps directly to YAML components, with two custom external components for the things ESPHome doesn't ship with: USB-MIDI and 2-channel jog quadrature. The build is resilient — it never reboots when WiFi or Home Assistant is unreachable, so the deck works as a plain USB-MIDI controller at any gig.

---

## Why ESPHome

| Ergonomic | Mechanical |
|---|---|
| YAML config maps 1:1 to the per-subsystem wiring docs | OTA at home, plug-and-play away |
| Built-in components for `cd74hc4067` mux, ADC, GPIO, ESP32 touch, light | Multi-SSID + AP fallback if home WiFi isn't there |
| Standard logging, sensor templating, debouncers, automation | Home Assistant integration is *optional* — nothing depends on it |
| Easy to extend with `external_components` (C++ alongside YAML) | Same firmware runs on the bench, at home, and at gigs |

## Project structure

```
firmware/
├── README.md            (this file)
├── cdj1000.yaml         main ESPHome config — board, WiFi/OTA, every component
├── secrets.yaml         WiFi creds, OTA password, API encryption key (gitignored)
├── secrets.yaml.example template — copy to secrets.yaml and edit
└── components/          custom external components
    ├── usb_midi/        class-compliant USB-MIDI via TinyUSB
    └── jog_quadrature/  S3 PCNT-peripheral 2-channel quadrature decoder
```

---

## Building

Install ESPHome locally (or use Docker):

```bash
pip install esphome
# or: pipx install esphome
```

Initial flash over USB:

```bash
cd firmware
esphome run cdj1000.yaml --device /dev/ttyACM0   # macOS/Linux
esphome run cdj1000.yaml --device COM3            # Windows
```

After the first flash, subsequent updates can go OTA when the deck is on the home network:

```bash
esphome run cdj1000.yaml   # auto-discovers via mDNS
```

---

## Resilience config (why the deck doesn't reboot away from home)

The defaults in ESPHome reboot the chip after ~5 minutes of no WiFi or no Home Assistant connection. For a gig deck that's catastrophic — the host laptop would lose the MIDI device mid-set. The fix is two lines:

```yaml
wifi:
  ssid: !secret home_ssid
  password: !secret home_password
  reboot_timeout: 0s           # NEVER reboot on WiFi disconnect

api:
  reboot_timeout: 0s           # NEVER reboot on HA disconnect
```

Plus a captive-portal AP fallback so you can re-credential it without a USB cable if you ever forget the password:

```yaml
wifi:
  # ... main config ...
  ap:
    ssid: "CDJ1000MK2-Setup"
    password: ""               # open AP for setup
```

Multi-SSID for travelling decks (optional):

```yaml
wifi:
  networks:
    - ssid: !secret home_ssid
      password: !secret home_password
      priority: 10
    - ssid: !secret studio_ssid
      password: !secret studio_password
      priority: 5
```

With this, the deck:
- Connects to home WiFi if available, gets HA + OTA
- Connects to studio WiFi if home isn't there
- Opens its own AP if nothing is there
- Never reboots based on connectivity — only on actual crash / power cycle
- Continues serving USB-MIDI to the host laptop regardless of any of that

## Custom components (what ESPHome doesn't ship)

### 1. `usb_midi` (REQUIRED for v0.1)

ESPHome has no native USB-MIDI class device. We add a small external component wrapping TinyUSB MIDI:

- Mounts as a class-compliant USB-MIDI device when plugged into the host
- Exposes a YAML action `usb_midi.send_note_on`, `usb_midi.send_cc`, `usb_midi.send_pitchbend`
- Receives MIDI from host into a queue (for display feedback later)

Scaffold: `components/usb_midi/usb_midi.h` + `usb_midi.cpp` + `__init__.py` (ESPHome config schema). ~300 lines of C++.

### 2. `jog_quadrature` (REQUIRED for v0.1)

ESPHome's `pulse_counter` is edge-count only — no direction. The ESP32-S3 PCNT peripheral does hardware quadrature decode natively (counts up on CW, down on CCW). We expose it as an ESPHome sensor:

```yaml
sensor:
  - platform: jog_quadrature
    name: "Jog Position"
    pin_a: GPIO4
    pin_b: GPIO5
    id: jog_pcnt
    update_interval: 10ms
```

- Reports signed delta since last read
- Uses ESP32-S3 PCNT hardware (zero CPU overhead)
- ~150 lines of C++ wrapping `driver/pulse_cnt.h`

### Roadmap

| Component | Purpose | Status |
|---|---|---|
| `usb_midi` | USB-MIDI class device via TinyUSB | ✅ v0.1 implementation — see [`components/usb_midi/README.md`](components/usb_midi/README.md) |
| `jog_quadrature` | 2-channel quadrature decode via PCNT | 🚧 next |
| `cdj_protocol` (v0.2) | OEM panel↔main frame emitter (jog cursor / mode LEDs on OEM VFD) | 📋 deferred — see `../docs/wiring/08-display.md` |

---

## How this firmware maps to the wiring docs

Every YAML block in `cdj1000.yaml` is commented with the subsystem doc it implements:

| YAML section | Subsystem doc |
|---|---|
| `sensor: -platform: jog_quadrature` | [`01-jog-encoder.md`](../docs/wiring/01-jog-encoder.md) |
| `binary_sensor: -platform: esp32_touch` | [`02-jog-touch.md`](../docs/wiring/02-jog-touch.md) — Path A |
| `sensor: -platform: adc` (GPIO 1) | [`03-pitch-fader.md`](../docs/wiring/03-pitch-fader.md) |
| `cd74hc4067:` + `binary_sensor: -platform: cd74hc4067` | [`04-buttons-mux.md`](../docs/wiring/04-buttons-mux.md) |
| `binary_sensor: -platform: gpio` (PLAY/CUE/VINYL/EJECT) | `05-discrete.md` (TBD) |
| `output: -platform: gpio` (LEDs via MOSFETs) | [`06-leds.md`](../docs/wiring/06-leds.md) |

---

## License + attribution

GPL-3.0, same as the rest of the repo. Custom components live under `firmware/components/` and ship with the repo.

If we pull in any third-party external component (e.g. an existing community quadrature decoder for ESPHome), we'll vendor it under `components/third-party/` with its license file intact.
