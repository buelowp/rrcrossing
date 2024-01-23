#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <functional>

#define ONE_MILLISECOND         1000
#define ONE_SECOND              (ONE_MILLISECOND * 1000)
#define FIVE_SECONDS            (ONE_SECOND * 5)

#define INVALID_ALARM           -10000

class StateMachine {
public:
    StateMachine();
    StateMachine(std::function<void(bool)> lightSwitch, std::function<void(bool)> gateControl);
    ~StateMachine();

    typedef enum DIRECTIONS:int {
        EAST = 0,
        WEST = 1,
        REMOTE = 2,
        STOPPED = 3,
    } directions;

    typedef enum STATES:int {
        ON = 0,
        OFF,
        SIGNALGATEUP,
        GATEUP,
        SIGNALGATEDOWN,
        GATEDOWN,
    } states;

    typedef enum EVENTS:int {
        EASTBOUND = 0,
        WESTBOUND,
        LEAVINGEASTBOUND,
        LEAVINGWESTBOUND,
        SIBLINGACTIVE,
        SIBLINGINACTIVE,
        RAISEGATE,
        GATERAISED,
        LOWERGATEDONE,
    } events;

    void setCallbacks(std::function<void(bool)> lightSwitch, std::function<void(bool)> gateControl);
    void handleEvent(events event);
    bool stillActive();
    void setAlarmHandleValue(int32_t value);

private:
    void turnOn();
    void turnOff();
    void raiseGate();

    std::function<void(bool)> m_lightSwitch;
    std::function<void(bool)> m_gateControl;
    states m_state;
    bool m_isActive;
    bool m_continueWaitingForExit;
    directions m_direction;
    bool m_localSensorActive;
    bool m_remoteSensorActive;
    alarm_id_t m_endAlarm;
};
