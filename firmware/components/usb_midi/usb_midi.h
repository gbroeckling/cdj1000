#pragma once

#include "esphome/core/component.h"
#include <string>
#include <cstdint>

namespace esphome {
namespace usb_midi {

// Class-compliant USB-MIDI device wrapping TinyUSB's `tud_midi_*` API.
//
// Lifetime: created once at boot by ESPHome. setup() installs the TinyUSB
// driver with MIDI class enabled. loop() pumps tud_task() so the USB stack
// services control transfers and IN/OUT endpoints.
//
// All send_*() calls are safe to invoke before enumeration completes — they
// silently drop the message if `tud_midi_mounted()` is false. That makes
// them safe to call from boot-time on_press hooks without ordering hassle.
class UsbMidi : public Component {
 public:
  // ESPHome lifecycle
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;
  void dump_config() override;

  // Config setters (called from generated to_code() in __init__.py)
  void set_manufacturer(const std::string &m) { manufacturer_ = m; }
  void set_product(const std::string &p)      { product_      = p; }
  void set_serial(const std::string &s)       { serial_       = s; }
  void set_vid(uint16_t v)                    { vid_          = v; }
  void set_pid(uint16_t p)                    { pid_          = p; }

  // String accessors used by usb_descriptors.c
  const char *manufacturer() const { return manufacturer_.c_str(); }
  const char *product()      const { return product_.c_str();      }
  const char *serial()       const { return serial_.c_str();       }
  uint16_t    vid()          const { return vid_; }
  uint16_t    pid()          const { return pid_; }

  // Channel: 0..15  (MIDI ch 1..16)
  // Note:    0..127
  // Velocity 0..127
  // CC#:     0..127
  // CC val:  0..127
  // Pitchbend: -8192..+8191 (14-bit signed)
  void send_note_on (uint8_t channel, uint8_t note, uint8_t velocity);
  void send_note_off(uint8_t channel, uint8_t note);
  // Convenience: velocity 0 → note off, else note on.
  void send_note    (uint8_t channel, uint8_t note, uint8_t velocity);
  void send_cc      (uint8_t channel, uint8_t cc,   uint8_t value);
  void send_pitchbend(uint8_t channel, int16_t value);

  // True once the host has enumerated us and the MIDI endpoints are open.
  bool is_mounted() const;

 protected:
  // Static accessor so the C-language usb_descriptors.c can fetch the
  // currently-instantiated UsbMidi to read string descriptors.
  friend const UsbMidi *get_usb_midi_instance();
  static UsbMidi *instance_;

  std::string manufacturer_;
  std::string product_;
  std::string serial_;
  uint16_t    vid_{0x303A};
  uint16_t    pid_{0x4D49};
  bool        initialized_{false};
};

// C-friendly accessor used by usb_descriptors.c
extern "C" const char *usb_midi_string_manufacturer();
extern "C" const char *usb_midi_string_product();
extern "C" const char *usb_midi_string_serial();
extern "C" uint16_t    usb_midi_get_vid();
extern "C" uint16_t    usb_midi_get_pid();

}  // namespace usb_midi
}  // namespace esphome
