// USB descriptors for the cdj1000 USB-MIDI device.
//
// TinyUSB calls these tud_descriptor_*_cb() functions to read the
// device, configuration, and string descriptors. We use TinyUSB's
// helper macros (TUD_CONFIG_DESCRIPTOR + TUD_MIDI_DESCRIPTOR) so the
// full USB-MIDI 1.0 spec class/subclass/endpoint hierarchy is built
// for us — no need to spell out the audio-control + audio-streaming
// + MIDI element descriptors by hand.
//
// One IN + one OUT bulk endpoint, 64-byte FS packets, one virtual
// MIDI cable. Plenty for a controller emitting Note/CC/PB events.

#include "tusb.h"

// Strings/IDs come from the C++ side (UsbMidi instance).
extern const char *usb_midi_string_manufacturer(void);
extern const char *usb_midi_string_product(void);
extern const char *usb_midi_string_serial(void);
extern uint16_t    usb_midi_get_vid(void);
extern uint16_t    usb_midi_get_pid(void);

// ============================================================
// Device descriptor (returned by tud_descriptor_device_cb)
// ============================================================
static tusb_desc_device_t desc_device_storage = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,   // class per-interface
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A, // filled at runtime
    .idProduct          = 0x4D49, // filled at runtime
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) {
  desc_device_storage.idVendor  = usb_midi_get_vid();
  desc_device_storage.idProduct = usb_midi_get_pid();
  return (uint8_t const *) &desc_device_storage;
}

// ============================================================
// Configuration descriptor
// ============================================================
enum {
  ITF_NUM_MIDI = 0,
  ITF_NUM_MIDI_STREAMING,
  ITF_NUM_TOTAL,
};

#define EPNUM_MIDI_OUT  0x01  // host → device
#define EPNUM_MIDI_IN   0x81  // device → host

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

static uint8_t const desc_configuration[] = {
    // (numItf, strIdx, attribute, totalLen, bmAttr, mA)
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // (itfnum, stridx, epout, epin, epsize)
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0,
                        EPNUM_MIDI_OUT, EPNUM_MIDI_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void) index;
  return desc_configuration;
}

// ============================================================
// String descriptors
// ============================================================
// Index 0 = supported language (English/US 0x0409). Other indices
// reference our C++ string storage via the extern accessors above.
//
// Conversion to UTF-16LE happens in a static scratch buffer; TinyUSB
// allows that because it copies the bytes before returning to the
// host stack.

static uint16_t _desc_str[64];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;

  uint8_t chr_count = 0;

  if (index == 0) {
    _desc_str[1] = 0x0409;  // English (US)
    chr_count = 1;
  } else {
    const char *src = NULL;
    switch (index) {
      case 1: src = usb_midi_string_manufacturer(); break;
      case 2: src = usb_midi_string_product();      break;
      case 3: src = usb_midi_string_serial();       break;
      default: return NULL;
    }
    if (!src) return NULL;

    // ASCII → UTF-16LE
    size_t max_chars = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
    while (*src && chr_count < max_chars) {
      _desc_str[1 + chr_count] = (uint16_t) (unsigned char) *src++;
      ++chr_count;
    }
  }

  // Length byte | descriptor type byte combined into first word.
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}
