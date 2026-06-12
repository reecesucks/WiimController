#pragma once

#include <functional>
#include <vector>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

// Polled, software-debounced active-low buttons (one terminal -> GPIO,
// other terminal -> GND; internal pull-up keeps the line HIGH when released).
//
// Usage:
//   Buttons btns;
//   btns.add(GPIO_NUM_13, []{ /* prev pressed */ });
//   btns.begin();
//   // in a task loop:
//   while (true) { btns.poll(); vTaskDelay(pdMS_TO_TICKS(5)); }
class Buttons {
public:
    using Callback = std::function<void()>;

    void add(gpio_num_t pin, Callback onPress);
    void begin();
    void poll();

private:
    struct Entry {
        gpio_num_t pin;
        Callback   onPress;
        bool       lastReading     = true;   // true = HIGH = released
        bool       lastStable      = true;
        TickType_t lastChangeTicks = 0;
    };

    static constexpr TickType_t kDebounceTicks = pdMS_TO_TICKS(20);

    std::vector<Entry> _entries;
};
