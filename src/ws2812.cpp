#include "ws2812.h"

WS2812::WS2812(int pixels, int pin, bool enable, int enablePin) : m_pixels(pixels), m_pin(pin), m_brightness(255), m_pctBrightness(100.0)
{
    if (enable) {
        gpio_init(enablePin);
        gpio_set_dir(enablePin, GPIO_OUT);
        gpio_put(enablePin, true);
    }

    m_strip = (uint32_t*)malloc(m_pixels);
    clear();
    m_pio = pio0; /* the PIO used */
    uint offset = pio_add_program(m_pio, &ws2812_program);
    ws2812_program_init(m_pio, 0, offset, pin, 800000, false); /* initialize it for 800 kHz */
}

WS2812::~WS2812()
{
    free(m_strip);
}

void WS2812::show()
{
    for(int i=0; i < m_pixels; i++) { /* without DMA: writing one after each other */
        pio_sm_put_blocking(m_pio, 0, m_strip[i]);
    }
    sleep_ms(1);
}

void WS2812::clear()
{
    for (int i = 0; i < m_pixels; i++) {
        m_strip[i] = 0;
    }
}

void WS2812::setColor(uint32_t c)
{
    for (int i = 0; i < m_pixels; i++) {
        uint8_t red = ((c >> 16) & 0xff);
        uint8_t green = ((c >> 24) & 0xff);
        uint8_t blue = ((c >> 8) & 0xff);
        setColor(i, red, green, blue);
    }
}

void WS2812::setColor(int index, uint32_t c)
{
    if (index < m_pixels) {
        uint8_t red = ((c >> 16) & 0xff);
        uint8_t green = ((c >> 24) & 0xff);
        uint8_t blue = ((c >> 8) & 0xff);
        setColor(index, red, green, blue);    
    }
}

void WS2812::setColor(uint8_t red, uint8_t green, uint8_t blue)
{
    for (int i = 0; i < m_pixels; i++) {
        setColor(i, red, green, blue);
    }
}

void WS2812::setColor(int index, uint8_t red, uint8_t green, uint8_t blue)
{
    if (index < m_pixels) {
        red = ((uint32_t)red * m_pctBrightness) / 100;
        green = ((uint32_t)green * m_pctBrightness) / 100;
        blue = ((uint32_t)blue * m_pctBrightness) / 100;
        printf("%s: %d: %d:%d:%d:%d\n", __PRETTY_FUNCTION__, index, red, green, blue, m_pctBrightness);
        uint32_t c = ((uint32_t)(green)<<24) | ((uint32_t)(red)<<16) | ((uint32_t)(blue)<<8);
        m_strip[index] = c;
    }
}

void WS2812::color(int index, uint32_t &c)
{
    c = 0;
    if (index < m_pixels) {
        c = m_strip[index];
    }
}

void WS2812::color(int index, uint8_t &red, uint8_t &green, uint8_t &blue)
{
    if (index < m_pixels) {
        red = (m_strip[index] >> 16) & 0xff;
        green = (m_strip[index] >> 24) & 0xff;
        blue = (m_strip[index] >> 8) & 0xff;
    }
}

void WS2812::setBrightness(uint8_t brightness)
{
    m_brightness = brightness;
    m_pctBrightness = ((double)brightness / 255.0) * 100;
    printf("%s: b:%d m_b:%d m_pct:%d\n", __PRETTY_FUNCTION__, brightness, m_brightness, m_pctBrightness);
    for (int i = 0; i < m_pixels; i++) {
        uint8_t red = ((m_strip[i] >> 16) & 0xff);
        uint8_t green = ((m_strip[i] >> 24) & 0xff);
        uint8_t blue = ((m_strip[i] >> 8) & 0xff);
        setColor(i, red, green, blue);
    }
}

uint8_t WS2812::brightness()
{
    return m_brightness;
}