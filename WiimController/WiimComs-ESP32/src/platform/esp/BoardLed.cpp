#include "BoardLed.h"

namespace {
void offTrampoline(void* arg) {
    static_cast<BoardLed*>(arg)->off();
}
}  // namespace

BoardLed::BoardLed(gpio_num_t pin, bool activeHigh)
    : _pin(pin), _activeHigh(activeHigh) {}

BoardLed::~BoardLed() {
    if (_offTimer) {
        esp_timer_stop(_offTimer);
        esp_timer_delete(_offTimer);
    }
}

void BoardLed::begin() {
    gpio_reset_pin(_pin);
    gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
    off();

    esp_timer_create_args_t args = {};
    args.callback = &offTrampoline;
    args.arg      = this;
    args.name     = "boardLedOff";
    esp_timer_create(&args, &_offTimer);
}

void BoardLed::on() {
    gpio_set_level(_pin, _activeHigh ? 1 : 0);
}

void BoardLed::off() {
    gpio_set_level(_pin, _activeHigh ? 0 : 1);
}

void BoardLed::blink(uint32_t ms) {
    on();
    if (_offTimer) {
        // Reset the off timer if we're called again while still on.
        esp_timer_stop(_offTimer);
        esp_timer_start_once(_offTimer, static_cast<uint64_t>(ms) * 1000);
    }
}
