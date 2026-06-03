// TinyUSB compile-time configuration for the cdj1000 USB-MIDI device.
//
// Only the bits we actually use are enabled — keeps the device descriptor
// minimal and the firmware footprint small. CFG_TUD_MIDI is set via a
// build flag added in __init__.py rather than here, so the same header
// works whether the component is built standalone or via ESPHome codegen.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ---- Root settings ----
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU              OPT_MCU_ESP32S3
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS               OPT_OS_FREERTOS
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG            0
#endif

// ---- Device side ----
#ifndef CFG_TUD_ENABLED
#define CFG_TUD_ENABLED           1
#endif

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

// MIDI is enabled via -DCFG_TUD_MIDI=1 in __init__.py.
// RX / TX buffer sizes are also set there so the same numbers reach
// both this header and the .cpp/.c files at build time.

// All other classes off.
#ifndef CFG_TUD_CDC
#define CFG_TUD_CDC               0
#endif
#ifndef CFG_TUD_MSC
#define CFG_TUD_MSC               0
#endif
#ifndef CFG_TUD_HID
#define CFG_TUD_HID               0
#endif
#ifndef CFG_TUD_AUDIO
#define CFG_TUD_AUDIO             0
#endif
#ifndef CFG_TUD_VIDEO
#define CFG_TUD_VIDEO             0
#endif
#ifndef CFG_TUD_VENDOR
#define CFG_TUD_VENDOR            0
#endif

#ifdef __cplusplus
}
#endif
