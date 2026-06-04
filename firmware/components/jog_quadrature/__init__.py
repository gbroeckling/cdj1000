"""ESPHome external component: 2-channel quadrature decoder.

ESPHome's stock `pulse_counter` platform is edge-count only — it cannot
report direction. The ESP32-S3 PCNT (Pulse Counter) peripheral does
hardware quadrature decode natively: it counts up on clockwise rotation
and down on counter-clockwise, with zero CPU overhead.

This component exposes the PCNT unit as a normal ESPHome `Sensor` whose
published value is the **signed delta count** since the previous update.
That's exactly the format the MIDI jog-scratch wrapper wants. Lambdas
can also call `read_delta()` and `read_total()` directly on the
component id for tighter control.

Used by cdj1000.yaml. See firmware/README.md.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

CODEOWNERS = ["@gbroeckling"]
DEPENDENCIES = ["esp32"]

jog_quadrature_ns = cg.esphome_ns.namespace("jog_quadrature")
JogQuadrature = jog_quadrature_ns.class_(
    "JogQuadrature", cg.PollingComponent, sensor.Sensor
)

CONF_PIN_A = "pin_a"
CONF_PIN_B = "pin_b"
CONF_GLITCH_FILTER_NS = "glitch_filter_ns"
CONF_MODE = "mode"

QUADRATURE_MODES = {
    "X1": 1,  # count edges of CH A only
    "X2": 2,  # count edges of CH A on both rising and falling
    "X4": 4,  # full quadrature: edges of CH A AND CH B (4× resolution)
}


def _gpio_num(value):
    # Accept either an int (GPIO4) or the bare number; ESP32-S3 valid range.
    return cv.int_range(min=0, max=48)(value)


CONFIG_SCHEMA = (
    sensor.sensor_schema(
        JogQuadrature,
        accuracy_decimals=0,
    )
    .extend(
        {
            cv.Required(CONF_PIN_A): _gpio_num,
            cv.Required(CONF_PIN_B): _gpio_num,
            cv.Optional(CONF_GLITCH_FILTER_NS, default=1000): cv.positive_int,
            cv.Optional(CONF_MODE, default="X4"): cv.enum(
                QUADRATURE_MODES, upper=True
            ),
        }
    )
    .extend(cv.polling_component_schema("10ms"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_pin_a(config[CONF_PIN_A]))
    cg.add(var.set_pin_b(config[CONF_PIN_B]))
    cg.add(var.set_glitch_filter_ns(config[CONF_GLITCH_FILTER_NS]))
    cg.add(var.set_mode(QUADRATURE_MODES[config[CONF_MODE]]))
