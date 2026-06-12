#pragma once

#include <functional>

#include "driver/gpio.h"

// Polled quadrature decoder for a KY-040 / EC11 style rotary encoder.
// Detents are reported one at a time; the callback receives +1 for clockwise
// and -1 for counter-clockwise. Wire CLK/DT to GPIOs with internal pull-ups
// (both VCC and GND from the encoder still need to be connected).
class RotaryEncoder {
public:
    using DirCallback = std::function<void(int delta)>;

    RotaryEncoder(gpio_num_t clkPin, gpio_num_t dtPin, DirCallback onChange);

    void begin();
    void poll();

private:
    gpio_num_t  _clk;
    gpio_num_t  _dt;
    DirCallback _onChange;
    uint8_t     _state = 0;  // last (A,B) value
    int8_t      _accum = 0;  // accumulates sub-steps to a full detent
};
