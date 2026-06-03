#include "usb_midi.h"
#include "esphome/core/log.h"

extern "C" {
#include "tinyusb.h"
#include "tusb.h"
}

namespace esphome {
namespace usb_midi {

static const char *const TAG = "usb_midi";

UsbMidi *UsbMidi::instance_ = nullptr;

// USB MIDI 1.0 4-byte event packet helper.
// Cable Number (high nibble of byte 0) is always 0 — we expose a single
// virtual cable, which is what every common host treats as "Port 1".
static inline void midi_stream(const uint8_t *msg, size_t n) {
  if (!tud_midi_mounted()) {
    // Not yet enumerated or host has detached. Drop silently — the
    // controller still works for the rest of its inputs.
    return;
  }
  tud_midi_stream_write(/*cable=*/0, msg, n);
}

void UsbMidi::setup() {
  ESP_LOGCONFIG(TAG, "Initialising USB-MIDI");
  ESP_LOGCONFIG(TAG, "  Manufacturer: %s", this->manufacturer_.c_str());
  ESP_LOGCONFIG(TAG, "  Product:      %s", this->product_.c_str());
  ESP_LOGCONFIG(TAG, "  Serial:       %s", this->serial_.c_str());
  ESP_LOGCONFIG(TAG, "  VID:PID:      %04X:%04X", this->vid_, this->pid_);

  // Make this instance reachable from the C-language descriptor file.
  UsbMidi::instance_ = this;

  // esp_tinyusb installs the USB stack and starts the device task. The
  // descriptors come from usb_descriptors.c which TinyUSB calls back into
  // via the tud_descriptor_* hooks.
  tinyusb_config_t tusb_cfg = {};
  tusb_cfg.device_descriptor = nullptr;       // use TUD_DESC_VIA_CALLBACK
  tusb_cfg.string_descriptor = nullptr;
  tusb_cfg.string_descriptor_count = 0;
  tusb_cfg.external_phy = false;
  tusb_cfg.configuration_descriptor = nullptr;

  esp_err_t err = tinyusb_driver_install(&tusb_cfg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  this->initialized_ = true;
  ESP_LOGI(TAG, "USB-MIDI driver installed");
}

void UsbMidi::loop() {
  // esp_tinyusb runs its own FreeRTOS task; tud_task() is called there.
  // Nothing for us to do per ESPHome loop iteration — kept for future
  // hooks (incoming MIDI parsing, etc.).
}

float UsbMidi::get_setup_priority() const {
  // Bring the USB stack up before any sensor/actuator that might emit MIDI.
  return setup_priority::BUS;
}

void UsbMidi::dump_config() {
  ESP_LOGCONFIG(TAG, "USB-MIDI:");
  ESP_LOGCONFIG(TAG, "  Mounted: %s", tud_midi_mounted() ? "yes" : "no");
}

bool UsbMidi::is_mounted() const { return tud_midi_mounted(); }

void UsbMidi::send_note_on(uint8_t ch, uint8_t note, uint8_t vel) {
  uint8_t msg[3] = {
      static_cast<uint8_t>(0x90 | (ch & 0x0F)),
      static_cast<uint8_t>(note & 0x7F),
      static_cast<uint8_t>(vel & 0x7F),
  };
  midi_stream(msg, 3);
}

void UsbMidi::send_note_off(uint8_t ch, uint8_t note) {
  uint8_t msg[3] = {
      static_cast<uint8_t>(0x80 | (ch & 0x0F)),
      static_cast<uint8_t>(note & 0x7F),
      0x00,
  };
  midi_stream(msg, 3);
}

void UsbMidi::send_note(uint8_t ch, uint8_t note, uint8_t vel) {
  if (vel == 0) {
    this->send_note_off(ch, note);
  } else {
    this->send_note_on(ch, note, vel);
  }
}

void UsbMidi::send_cc(uint8_t ch, uint8_t cc, uint8_t val) {
  uint8_t msg[3] = {
      static_cast<uint8_t>(0xB0 | (ch & 0x0F)),
      static_cast<uint8_t>(cc & 0x7F),
      static_cast<uint8_t>(val & 0x7F),
  };
  midi_stream(msg, 3);
}

void UsbMidi::send_pitchbend(uint8_t ch, int16_t value) {
  // MIDI pitchbend is 14-bit unsigned, centred at 0x2000. We accept the
  // signed -8192..+8191 range and re-bias here.
  uint16_t pb = static_cast<uint16_t>(value + 8192);
  uint8_t msg[3] = {
      static_cast<uint8_t>(0xE0 | (ch & 0x0F)),
      static_cast<uint8_t>(pb & 0x7F),         // LSB
      static_cast<uint8_t>((pb >> 7) & 0x7F),  // MSB
  };
  midi_stream(msg, 3);
}

// ---- C-friendly accessors for usb_descriptors.c ----

extern "C" const char *usb_midi_string_manufacturer() {
  return UsbMidi::instance_ ? UsbMidi::instance_->manufacturer() : "cdj1000";
}
extern "C" const char *usb_midi_string_product() {
  return UsbMidi::instance_ ? UsbMidi::instance_->product() : "CDJ-1000MK2";
}
extern "C" const char *usb_midi_string_serial() {
  return UsbMidi::instance_ ? UsbMidi::instance_->serial() : "0001";
}
extern "C" uint16_t usb_midi_get_vid() {
  return UsbMidi::instance_ ? UsbMidi::instance_->vid() : 0x303A;
}
extern "C" uint16_t usb_midi_get_pid() {
  return UsbMidi::instance_ ? UsbMidi::instance_->pid() : 0x4D49;
}

}  // namespace usb_midi
}  // namespace esphome
