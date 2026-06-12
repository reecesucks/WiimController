#include "RotaryEncoder.h"

// Quadrature transition table — indexed by (oldA oldB newA newB).
// 0 = no movement or invalid (noise), +1 = clockwise sub-step, -1 = ccw.
static const int8_t kTransitions[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
};

RotaryEncoder::RotaryEncoder(gpio_num_t clkPin, gpio_num_t dtPin, DirCallback onChange)
    : _clk(clkPin), _dt(dtPin), _onChange(std::move(onChange)) {}

void RotaryEncoder::begin() {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << _clk) | (1ULL << _dt);
    cfg.mode         = GPIO_MODE_INPUT;
    cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&cfg);

    int a = gpio_get_level(_clk);
    int b = gpio_get_level(_dt);
    _state = static_cast<uint8_t>((a << 1) | b);
}

void RotaryEncoder::poll() {
    int a = gpio_get_level(_clk);
    int b = gpio_get_level(_dt);
    uint8_t now = static_cast<uint8_t>((a << 1) | b);
    if (now == _state) return;

    uint8_t idx = static_cast<uint8_t>(((_state & 0x3) << 2) | (now & 0x3));
    _accum += kTransitions[idx & 0xF];
    _state  = now;

    // Most KY-040 / EC11 encoders produce 4 sub-steps per physical detent.
    if (_accum >= 4) {
        _accum -= 4;
        if (_onChange) _onChange(+1);
    } else if (_accum <= -4) {
        _accum += 4;
        if (_onChange) _onChange(-1);
    }
}
