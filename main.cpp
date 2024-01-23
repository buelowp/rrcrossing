#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "ws2812.h"
#include "Servo.h"
#include "statemachine.h"

#define APP_ID          13

#define SEMAPHORE       27
#define SIBLING_ACTIVE        26
#define LED_R           17
#define LED_G           16
#define LED_B           25

#define SENSOR_EAST_ENTRY        28
#define SENSOR_WEST_EXIT        29
#define SENSOR_EAST_EXIT        6
#define SENSOR_WEST_ENTRY        7

#define NORTH_GATE          0
#define SOUTH_GATE          4

#define GATE_DOWN       0
#define GATE_UP         90

#define SIG_LEFT        1
#define SIG_RIGHT       2

bool g_cancelSignals;
int g_whichSignal;
int g_gateAngle;
int g_gateTarget;
Servo servo;
repeating_timer_t g_activeSignalTimer;
repeating_timer_t g_activeGateTimer;
StateMachine sm;

void set_led_color(bool r, bool g, bool b)
{
    gpio_put(LED_R, true);
    gpio_put(LED_G, true);
    gpio_put(LED_B, true);

    if (r) {
        gpio_put(LED_R, false);
    }
    if (b) {
        gpio_put(LED_B, false);
    }
    if (g) {
        gpio_put(LED_G, false);
    }
}

bool signals_active(repeating_timer_t *rt)
{
    if (g_cancelSignals) {
        g_cancelSignals = false;
        gpio_put(SIG_LEFT, false);
        gpio_put(SIG_RIGHT, false);
        set_led_color(false, false, false);
        return false;
    }

    switch (g_whichSignal) {
        case SIG_LEFT:
            g_whichSignal = SIG_RIGHT;
            gpio_put(SIG_LEFT, true);
            gpio_put(SIG_RIGHT, false);
            break;
        case SIG_RIGHT:
            g_whichSignal = SIG_LEFT;
            gpio_put(SIG_LEFT, false);
            gpio_put(SIG_RIGHT, true);
            break;
    }
    return true;
}

void trigger_event(uint gpio, uint32_t event_mask)
{
    switch (gpio) {
        case SENSOR_EAST_ENTRY:
            if (event_mask == GPIO_IRQ_EDGE_RISE)
                sm.handleEvent(StateMachine::events::EASTBOUND);
            break;
        case SENSOR_WEST_ENTRY:
            if (event_mask == GPIO_IRQ_EDGE_RISE)
                sm.handleEvent(StateMachine::events::WESTBOUND);
            break;
        case SENSOR_WEST_EXIT:
            if (event_mask == GPIO_IRQ_EDGE_FALL)
                sm.handleEvent(StateMachine::events::LEAVINGWESTBOUND);
            break;
        case SENSOR_EAST_EXIT:
            if (event_mask == GPIO_IRQ_EDGE_FALL)
                sm.handleEvent(StateMachine::events::LEAVINGEASTBOUND);
            break;
        case SIBLING_ACTIVE:
            if (event_mask == GPIO_IRQ_EDGE_RISE) {
                sm.handleEvent(StateMachine::events::SIBLINGACTIVE);
            }
            else {
                sm.handleEvent(StateMachine::events::SIBLINGINACTIVE);
            }
            break;
    }
}

void setup_gpio()
{
    printf("Initializing gpio.\n");

    gpio_init(SEMAPHORE);
    gpio_init(SIBLING_ACTIVE);
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_init(SENSOR_EAST_ENTRY);
    gpio_init(SENSOR_WEST_EXIT);
    gpio_init(SENSOR_EAST_EXIT);
    gpio_init(SENSOR_WEST_ENTRY);
    gpio_init(SIG_LEFT);
    gpio_init(SIG_RIGHT);

    gpio_set_dir(SEMAPHORE, GPIO_OUT);

    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_set_dir(SENSOR_EAST_ENTRY, GPIO_IN);
    gpio_set_dir(SENSOR_WEST_EXIT, GPIO_IN);
    gpio_set_dir(SENSOR_EAST_EXIT, GPIO_IN);
    gpio_set_dir(SENSOR_WEST_ENTRY, GPIO_IN);
    gpio_set_dir(SIG_LEFT, GPIO_OUT);
    gpio_set_dir(SIG_RIGHT, GPIO_OUT);
    gpio_set_dir(SIBLING_ACTIVE, GPIO_IN);

    gpio_pull_down(SENSOR_EAST_ENTRY);
    gpio_pull_down(SENSOR_WEST_EXIT);
    gpio_pull_down(SENSOR_EAST_EXIT);
    gpio_pull_down(SENSOR_WEST_ENTRY);
    gpio_pull_down(SIBLING_ACTIVE);

    gpio_set_irq_enabled_with_callback(SENSOR_EAST_ENTRY, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, trigger_event);
    gpio_set_irq_enabled_with_callback(SENSOR_WEST_EXIT, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, trigger_event);
    gpio_set_irq_enabled_with_callback(SENSOR_EAST_EXIT, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, trigger_event);
    gpio_set_irq_enabled_with_callback(SENSOR_WEST_ENTRY, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, trigger_event);
    gpio_set_irq_enabled_with_callback(SIBLING_ACTIVE, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, trigger_event);

    servo.attach(NORTH_GATE);
    servo.write(GATE_DOWN);
}

void led_control(bool state)
{
    printf("%s: state:%s\n", __PRETTY_FUNCTION__, state ? "true" : "false");
    if (state) {
        g_cancelSignals = false;
        gpio_put(SEMAPHORE, true);
        add_repeating_timer_ms(1000, signals_active, NULL, &g_activeSignalTimer);
    }
    else {
        g_cancelSignals = true;
        gpio_put(SEMAPHORE, false);
    }
}

bool gate_action(repeating_timer_t *rt)
{
    if (g_gateTarget == GATE_UP) {
        if (g_gateAngle <= GATE_UP) {
            servo.write(++g_gateAngle);
            return true;
        }
        else {
            sm.handleEvent(StateMachine::events::GATERAISED);
        }
    }
    else if (g_gateTarget == GATE_DOWN) {
        if (g_gateAngle >= GATE_DOWN) {
            servo.write(--g_gateAngle);
            return true;
        }
    }

    return false;
}

void gate_control(bool state)
{
    printf("%s: state:%s\n", __PRETTY_FUNCTION__, state ? "true" : "false");
    if (state) {
        g_gateTarget = GATE_DOWN;
    }
    else {
        g_gateTarget = GATE_UP;
    }
    add_repeating_timer_ms(125, gate_action, NULL, &g_activeGateTimer);
}

int main() 
{
    bool triggerEvent = false;
    sm.setCallbacks(led_control, gate_control);
    stdio_init_all();
    sleep_ms(5000);
    setup_gpio();
    set_led_color(false, false, false);
    
    printf("Starting main for APP_ID: %d\n", APP_ID);

    g_whichSignal = SIG_LEFT;
    g_cancelSignals = false;
    g_gateAngle = GATE_DOWN;
    alarm_pool_create(0, 2);

    while (1) {
        tight_loop_contents();
    }

    return 0;
}