# jog_quadrature — ESPHome external component

Hardware 2-channel quadrature decoder for the ESP32-S3 PCNT peripheral,
exposed to ESPHome as a `sensor` platform.

## Why this exists

ESPHome's stock `pulse_counter` platform counts edges but cannot report
**direction** — it has no way to read CH B to decide whether the wheel
is turning clockwise or counter-clockwise. For a CDJ-style jog wheel
that's a non-starter; you need to know whether the user is scratching
forward or backward.

The S3's PCNT peripheral handles quadrature decode in hardware, with
zero CPU overhead even at high jog speeds, and with optional analog
glitch filtering. This component wires that up.

## YAML usage

```yaml
external_components:
  - source: components/jog_quadrature

sensor:
  - platform: jog_quadrature
    name: "Jog Position Delta"
    id: jog_delta
    pin_a: GPIO4           # CH A from TXS0108E B-side (Lane 1)
    pin_b: GPIO5           # CH B from TXS0108E B-side
    update_interval: 10ms  # 100 Hz publish rate
    # Optional:
    glitch_filter_ns: 1000 # ignore pulses shorter than 1 µs
    mode: X4               # full quadrature (default; also X1, X2)
    on_value:
      then:
        - lambda: |-
            // x is the signed delta count since the previous update.
            // Map to a Traktor jog-scratch CC centred at 64.
            int v = 64 + int(x);
            if (v < 0)   v = 0;
            if (v > 127) v = 127;
            id(usb_midi_dev).send_cc(0, 31, v);
```

## C++ accessors from lambdas

If you want a tighter loop than the `on_value` automation gives:

```yaml
interval:
  - interval: 5ms
    then:
      - lambda: |-
          int delta = id(jog_delta).read_delta();
          if (delta != 0) {
            id(usb_midi_dev).send_cc(0, 31, 64 + delta);
          }
```

| Method | Returns | Notes |
|---|---|---|
| `read_delta()` | `int` | signed count since last read; resets counter |
| `read_total()` | `int64_t` | running total from boot (refreshed before return) |

`read_delta()` and the auto-publish in `update()` share the underlying
PCNT unit — calling one resets the count for the other, so pick one
read path per loop and stick with it.

## Decode mode

Default is **X4** (full quadrature): both edges of CH A *and* both edges
of CH B contribute counts, with the other channel's level deciding
direction. With the OEM 135 PPR optical encoder that gives 540 counts
per revolution at the S3.

| Mode | What gets counted | Resolution vs encoder PPR |
|---|---|---|
| X1 | rising edges of CH A only, direction from CH B level | 1× |
| X2 | both edges of CH A, direction from CH B | 2× |
| X4 | both edges of CH A AND both edges of CH B | 4× |

For Traktor jog scratch X4 is strictly better — finer granularity =
smoother scratch feel. The PCNT peripheral handles all three at the
same near-zero CPU cost, so there's no reason to drop below X4 unless
you're chasing a specific MIDI mapping.

## Glitch filter

`glitch_filter_ns` (default **1000 ns / 1 µs**) tells PCNT to ignore
pulses shorter than that on either input. Optical encoders on a long
ribbon through TXS0108E can pick up ringing at the level-shifter; 1 µs
is well above any legitimate encoder edge (the OEM jog at maximum
scratch is < 100 kHz, i.e. 5 µs half-period). Bump higher only if
you see phantom counts at rest.

Set to `0` to disable filtering entirely.

## Wiring (matches docs/wiring/01-jog-encoder.md)

```
OEM JOGB encoder (5 V)  →  TXS0108E A-side
                           TXS0108E B-side (3.3 V)
                           ├─ B1 → GPIO 4 (CH A, this component's pin_a)
                           └─ B2 → GPIO 5 (CH B, this component's pin_b)
```

No additional parts on the carrier PCB — the level shifter from Lane 1
of the master wiring map is the only thing between the encoder and the
S3.

## Sanity log

`dump_config()` prints the pin assignment, mode, and glitch filter at
boot:

```
[C][jog_quadrature:182]: Jog Quadrature:
[C][jog_quadrature:183]:   Pin A:         GPIO4
[C][jog_quadrature:184]:   Pin B:         GPIO5
[C][jog_quadrature:185]:   Mode:          X4
[C][jog_quadrature:186]:   Glitch filter: 1000 ns
[C][jog_quadrature:187]:   Sensor 'Jog Position Delta'
```

If the encoder doesn't move the sensor: check pin assignment first;
swap pin_a and pin_b if direction is inverted (or flip the edge action
in `jog_quadrature.cpp`).

## License

GPL-3.0, same as the rest of the repo.
