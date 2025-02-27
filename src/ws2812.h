#include <cinttypes>
#include <cstdlib>
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "ws2812.pio.h"

class WS2812 {
public:
    WS2812(int pixels, int pin, bool enable = false, int enablePin = 11);
    ~WS2812();

    void clear();
    void show();
    void setColor(uint32_t c);
    void setColor(int index, uint32_t c);
    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void setColor(int index, uint8_t red, uint8_t green, uint8_t blue);
    void color(int index, uint32_t &c);
    void color(int index, uint8_t &red, uint8_t &green, uint8_t &blue);
    void setBrightness(uint8_t bright);
    uint8_t brightness();

private:
    uint32_t *m_strip;
    uint8_t m_brightness;
    uint8_t m_pctBrightness;
    int m_pixels;
    int m_pin;
    PIO m_pio;
};