#include "Buttons.h"

#include "freertos/task.h"

void Buttons::add(gpio_num_t pin, Callback onPress) {
    _entries.push_back({pin, std::move(onPress)});
}

void Buttons::begin() {
    for (auto& e : _entries) {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = (1ULL << e.pin);
        cfg.mode         = GPIO_MODE_INPUT;
        cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&cfg);

        bool level = (gpio_get_level(e.pin) != 0);
        e.lastReading     = level;
        e.lastStable      = level;
        e.lastChangeTicks = xTaskGetTickCount();
    }
}

void Buttons::poll() {
    TickType_t now = xTaskGetTickCount();
    for (auto& e : _entries) {
        bool reading = (gpio_get_level(e.pin) != 0);
        if (reading != e.lastReading) {
            e.lastReading     = reading;
            e.lastChangeTicks = now;
        }
        if ((now - e.lastChangeTicks) >= kDebounceTicks &&
            reading != e.lastStable) {
            e.lastStable = reading;
            // Falling edge = pressed (active-low with pull-up).
            if (!reading && e.onPress) {
                e.onPress();
            }
        }
    }
}
