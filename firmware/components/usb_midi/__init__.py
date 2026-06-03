"""ESPHome external component: USB-MIDI class device via TinyUSB.

Mounts the ESP32-S3 as a standard class-compliant USB-MIDI device when
plugged into a host (no driver install needed on macOS / Windows / Linux /
iPadOS). Exposes lambda-callable methods for sending Note On/Off, CC, and
Pitchbend so the rest of the ESPHome YAML can wire button/jog/fader events
straight into MIDI without any extra C++.

Used by cdj1000.yaml. See firmware/README.md.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32
from esphome.const import CONF_ID

CODEOWNERS = ["@gbroeckling"]
DEPENDENCIES = ["esp32"]
MULTI_CONF = False

usb_midi_ns = cg.esphome_ns.namespace("usb_midi")
UsbMidi = usb_midi_ns.class_("UsbMidi", cg.Component)

CONF_MANUFACTURER = "manufacturer"
CONF_PRODUCT = "product"
CONF_SERIAL = "serial"
CONF_VID = "vid"
CONF_PID = "pid"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UsbMidi),
        cv.Optional(CONF_MANUFACTURER, default="cdj1000 project"): cv.string,
        cv.Optional(CONF_PRODUCT, default="CDJ-1000MK2 controller"): cv.string,
        cv.Optional(CONF_SERIAL, default="CDJ1000MK2-0001"): cv.string,
        # Espressif's USB VID is 0x303A; pair with a per-project PID.
        # See https://github.com/espressif/usb-pids for the allocated range.
        cv.Optional(CONF_VID, default=0x303A): cv.hex_uint16_t,
        cv.Optional(CONF_PID, default=0x4D49): cv.hex_uint16_t,  # 'MI' ascii
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_manufacturer(config[CONF_MANUFACTURER]))
    cg.add(var.set_product(config[CONF_PRODUCT]))
    cg.add(var.set_serial(config[CONF_SERIAL]))
    cg.add(var.set_vid(config[CONF_VID]))
    cg.add(var.set_pid(config[CONF_PID]))

    # Pull in TinyUSB as an ESP-IDF managed component.
    esp32.add_idf_component(
        name="espressif/esp_tinyusb",
        ref="1.4.5",
    )

    # Enable MIDI class in TinyUSB's compile-time config.
    cg.add_build_flag("-DCFG_TUD_MIDI=1")
    cg.add_build_flag("-DCFG_TUD_MIDI_RX_BUFSIZE=64")
    cg.add_build_flag("-DCFG_TUD_MIDI_TX_BUFSIZE=64")
    cg.add_build_flag("-DCFG_TUD_MIDI_EP_BUFSIZE=64")
