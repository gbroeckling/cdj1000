#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include <cstdint>

extern "C" {
#include "driver/pulse_cnt.h"
}

namespace esphome {
namespace jog_quadrature {

// Hardware quadrature decoder built on the ESP32-S3 PCNT peripheral.
//
// One PCNT unit, two channels in X4 mode (count edges of CH A AND CH B,
// with the other pin's level controlling the direction). This yields
// 4× the resolution of the underlying optical encoder (so 135 PPR on
// the OEM jog → 540 counts/rev at the S3).
//
// On each ESPHome update tick we:
//   1. read the PCNT counter
//   2. clear it
//   3. accumulate the delta into total_
//   4. publish the delta as the Sensor's state
//
// Lambdas can also call read_delta() / read_total() directly for tighter
// MIDI-emit loops.
class JogQuadrature : public PollingComponent, public sensor::Sensor {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;

  // Config setters (called from generated to_code() in __init__.py)
  void set_pin_a(int p)                  { pin_a_ = p; }
  void set_pin_b(int p)                  { pin_b_ = p; }
  void set_glitch_filter_ns(uint32_t ns) { glitch_filter_ns_ = ns; }
  void set_mode(int m)                   { mode_ = m; }

  // Lambda-callable: signed delta count since last read. Resets the
  // PCNT counter as a side effect, so successive calls return only
  // newly-arrived edges.
  int read_delta();

  // Lambda-callable: running total count from boot. Folds in any
  // newly-arrived delta before returning, so it's always current.
  int64_t read_total();

 protected:
  int pin_a_{-1};
  int pin_b_{-1};
  uint32_t mode_{4};
  uint32_t glitch_filter_ns_{1000};

  pcnt_unit_handle_t    unit_{nullptr};
  pcnt_channel_handle_t chan_a_{nullptr};
  pcnt_channel_handle_t chan_b_{nullptr};

  int64_t total_{0};
  bool initialized_{false};
};

}  // namespace jog_quadrature
}  // namespace esphome
