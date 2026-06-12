#pragma once

#include "driver/gpio.h"
#include "esp_timer.h"

// Tiny wrapper for the dev board's built-in LED.
// Active-high by default (matches the blue LED on most ESP32-WROOM-32 dev kits).
// blink() is non-blocking: it turns the LED on now and schedules an off via
// an esp_timer one-shot, so the caller doesn't pause.
class BoardLed {
public:
    explicit BoardLed(gpio_num_t pin = GPIO_NUM_2, bool activeHigh = true);
    ~BoardLed();

    BoardLed(const BoardLed&)            = delete;
    BoardLed& operator=(const BoardLed&) = delete;

    void begin();
    void on();
    void off();
    void blink(uint32_t ms = 60);

private:
    gpio_num_t       _pin;
    bool             _activeHigh;
    esp_timer_handle_t _offTimer = nullptr;
};
