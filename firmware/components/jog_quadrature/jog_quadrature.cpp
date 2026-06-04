#include "jog_quadrature.h"
#include "esphome/core/log.h"

namespace esphome {
namespace jog_quadrature {

static const char *const TAG = "jog_quadrature";

// PCNT counter is 16-bit signed; we deliberately set the wrap limits to
// the full range so an `accum_count` enabled unit keeps the running
// total across wraps. Any update_interval that reads frequently enough
// to keep delta under 32 k counts is safe — at 540 counts/rev that's
// ~60 revolutions per update window, which is well beyond any DJ jog.
static constexpr int kPcntHighLimit =  32767;
static constexpr int kPcntLowLimit  = -32768;

void JogQuadrature::setup() {
  ESP_LOGCONFIG(TAG, "Bringing up PCNT on GPIO%d (CH A) / GPIO%d (CH B), mode X%u",
                pin_a_, pin_b_, mode_);

  pcnt_unit_config_t unit_cfg = {};
  unit_cfg.high_limit = kPcntHighLimit;
  unit_cfg.low_limit  = kPcntLowLimit;
  unit_cfg.flags.accum_count = true;  // accumulate count across high/low wraps

  esp_err_t err = pcnt_new_unit(&unit_cfg, &unit_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "pcnt_new_unit failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  if (glitch_filter_ns_ > 0) {
    pcnt_glitch_filter_config_t filter_cfg = {};
    filter_cfg.max_glitch_ns = glitch_filter_ns_;
    err = pcnt_unit_set_glitch_filter(unit_, &filter_cfg);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "glitch filter set failed: %s", esp_err_to_name(err));
    }
  }

  // ---- Channel A ----
  // Edge events come from CH A; CH B's level decides the direction.
  pcnt_chan_config_t chan_a_cfg = {};
  chan_a_cfg.edge_gpio_num  = pin_a_;
  chan_a_cfg.level_gpio_num = pin_b_;

  err = pcnt_new_channel(unit_, &chan_a_cfg, &chan_a_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "pcnt_new_channel A failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  // While CH B is LOW:  rising edge on CH A → +1, falling edge → −1
  // While CH B is HIGH: rising edge on CH A → −1, falling edge → +1 (inverted)
  pcnt_channel_set_edge_action(
      chan_a_,
      PCNT_CHANNEL_EDGE_ACTION_DECREASE,   // negative-going edge
      PCNT_CHANNEL_EDGE_ACTION_INCREASE);  // positive-going edge
  pcnt_channel_set_level_action(
      chan_a_,
      PCNT_CHANNEL_LEVEL_ACTION_KEEP,      // CH B low
      PCNT_CHANNEL_LEVEL_ACTION_INVERSE);  // CH B high → flip direction

  // ---- Channel B (X4 only) ----
  // Same trick with the pins swapped, so we count edges on CH B too.
  // X1 = CH A edges only, no direction sensing — included for completeness.
  // X2 = CH A both edges, CH B direction — actually achieved by configuring
  //      channel A with both edge actions (already done above).
  // X4 = CH A both edges + CH B both edges → 4× the OEM PPR.
  if (mode_ >= 4) {
    pcnt_chan_config_t chan_b_cfg = {};
    chan_b_cfg.edge_gpio_num  = pin_b_;
    chan_b_cfg.level_gpio_num = pin_a_;

    err = pcnt_new_channel(unit_, &chan_b_cfg, &chan_b_);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "pcnt_new_channel B failed (continuing in X2): %s",
               esp_err_to_name(err));
    } else {
      pcnt_channel_set_edge_action(
          chan_b_,
          PCNT_CHANNEL_EDGE_ACTION_INCREASE,
          PCNT_CHANNEL_EDGE_ACTION_DECREASE);
      pcnt_channel_set_level_action(
          chan_b_,
          PCNT_CHANNEL_LEVEL_ACTION_KEEP,
          PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    }
  }

  pcnt_unit_enable(unit_);
  pcnt_unit_clear_count(unit_);
  pcnt_unit_start(unit_);

  initialized_ = true;
  ESP_LOGI(TAG, "PCNT ready");
}

void JogQuadrature::update() {
  if (!initialized_) return;
  int delta = this->read_delta();
  this->publish_state(static_cast<float>(delta));
}

int JogQuadrature::read_delta() {
  if (!initialized_) return 0;
  int delta = 0;
  pcnt_unit_get_count(unit_, &delta);
  pcnt_unit_clear_count(unit_);
  total_ += delta;
  return delta;
}

int64_t JogQuadrature::read_total() {
  if (initialized_) {
    int delta = 0;
    pcnt_unit_get_count(unit_, &delta);
    pcnt_unit_clear_count(unit_);
    total_ += delta;
  }
  return total_;
}

void JogQuadrature::dump_config() {
  ESP_LOGCONFIG(TAG, "Jog Quadrature:");
  ESP_LOGCONFIG(TAG, "  Pin A:           GPIO%d", pin_a_);
  ESP_LOGCONFIG(TAG, "  Pin B:           GPIO%d", pin_b_);
  ESP_LOGCONFIG(TAG, "  Mode:            X%u",   mode_);
  ESP_LOGCONFIG(TAG, "  Glitch filter:   %u ns", glitch_filter_ns_);
  LOG_SENSOR("  ", "Sensor", this);
}

float JogQuadrature::get_setup_priority() const {
  return setup_priority::HARDWARE;  // before higher-level sensors
}

}  // namespace jog_quadrature
}  // namespace esphome
