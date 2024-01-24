#include "statemachine.h"

static int64_t enable_semaphore(alarm_id_t id, void *user_data)
{
    StateMachine *sm = (StateMachine*)user_data;
    sm->enableSemaphore();
    return 0;
}

static int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    StateMachine *sm = (StateMachine*)user_data;

    printf("Handling alarm %d, and still active is %d\n", id, sm->stillActive());
    if (sm->stillActive()) {
        return ONE_SECOND;
    }
    else {
        sm->handleEvent(StateMachine::events::RAISEGATE);
    }
    sm->setAlarmHandleValue(INVALID_ALARM);
    return 0;
}

StateMachine::StateMachine()
{
    m_state = OFF;
    m_isActive = false;
    m_direction = STOPPED;
    m_localSensorActive = false;
    m_endAlarm = INVALID_ALARM;
    m_continueWaitingForExit = false;
    setLedColor(false, false, false);
}

StateMachine::StateMachine(std::function<void(bool)> lightSwitch, std::function<void(bool)> gateControl, std::function<void(bool)> semaphoreControl)
{
    if (lightSwitch)
        m_lightSwitch = lightSwitch;

    if (gateControl)
        m_gateControl = gateControl;

    if (semaphoreControl)
        m_semControl = semaphoreControl;

    m_state = OFF;
    m_isActive = false;
    m_direction = STOPPED;
    m_localSensorActive = false;
    m_endAlarm = INVALID_ALARM;
    m_continueWaitingForExit = false;
    setLedColor(false, false, false);
}

StateMachine::~StateMachine()
{
}

void StateMachine::setLedColor(bool red, bool green, bool blue)
{
    gpio_put(colors::red, !red);
    gpio_put(colors::green, !green);
    gpio_put(colors::blue, !blue);
}

void StateMachine::setAlarmHandleValue(int32_t value) 
{
    m_endAlarm = value;
}

bool StateMachine::stillActive()
{
    return m_remoteSensorActive;
}

void StateMachine::setCallbacks(std::function<void(bool)> lightSwitch, std::function<void(bool)> gateControl, std::function<void(bool)> semaphoreControl)
{
    if (lightSwitch)
        m_lightSwitch = lightSwitch;

    if (gateControl)
        m_gateControl = gateControl;

    if (semaphoreControl)
        m_semControl = semaphoreControl;

    setLedColor(false, true, false);   
}

void StateMachine::raiseGate()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_gateControl(false);
    m_semControl(false);
    m_state = SIGNALGATEUP;
    setLedColor(true, true, false);
}

void StateMachine::turnOn()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_state = ON;
    m_lightSwitch(true);
    m_gateControl(true);
    m_semControl(true);
    if (m_remoteSensorActive)
        setLedColor(false, false, true);
    else
        setLedColor(true, false, false);
}

void StateMachine::turnOff()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_lightSwitch(false);
    m_state = OFF;
    m_direction = STOPPED;
    setLedColor(true, false, false);
}

void StateMachine::handleEvent(events event)
{
    switch (event) {
        case EASTBOUND:
            printf("%s: EASTBOUND: %d\n", __PRETTY_FUNCTION__, m_state);
            if (m_state == OFF) {
                m_localSensorActive = true;
                m_direction = EAST;
                turnOn();
            }
            break;
        case WESTBOUND:
            printf("%s: WESTBOUND: %d\n", __PRETTY_FUNCTION__, m_state);
            if (m_state == OFF) {
                m_localSensorActive = true;
                m_direction = WEST;
                turnOn();
            }
            break;
        case SIBLINGACTIVE:
            printf("%s: SIBLINGACTIVE: state:%d, remote: %s\n", __PRETTY_FUNCTION__, m_state, m_remoteSensorActive ? "true" : "false");
            if (m_state == OFF) {
                m_remoteSensorActive = true;
                turnOn();
            }
            break;
        case LEAVINGEASTBOUND:
            printf("%s: LEAVINGEASTBOUND: direction:%d, state:%d\n", __PRETTY_FUNCTION__, m_direction, m_state);
            if (m_direction == EAST && m_state == ON) {
                if (m_endAlarm == INVALID_ALARM) {
                    m_endAlarm = add_alarm_in_us(FIVE_SECONDS, alarm_callback, (void*)this, true);
                }
                else {
                    if (cancel_alarm(m_endAlarm)) {
                        printf("%s: Canceling alarm %d\n", m_endAlarm);
                        m_endAlarm = add_alarm_in_us(FIVE_SECONDS, alarm_callback, (void*)this, true);
                    }
                }
            }
            break;
        case LEAVINGWESTBOUND:
            printf("%s: LEAVINGWESTBOUND: direction:%d, state:%d\n", __PRETTY_FUNCTION__, m_direction, m_state);
            if (m_direction == WEST && m_state == ON) {
                if (m_endAlarm == INVALID_ALARM) {
                    m_endAlarm = add_alarm_in_us(FIVE_SECONDS, alarm_callback, (void*)this, true);
                }
                else {
                    if (cancel_alarm(m_endAlarm)) {
                        printf("%s: Canceling alarm %d\n", __PRETTY_FUNCTION__, m_endAlarm);
                        m_endAlarm = add_alarm_in_us(FIVE_SECONDS, alarm_callback, (void*)this, true);
                    }
                }
            }
            break;
        case SIBLINGINACTIVE:
            printf("%s: SIBLINGINACTIVE: state:%d, active:%d\n", __PRETTY_FUNCTION__, m_state, m_remoteSensorActive);
            if (m_remoteSensorActive) {
                if (m_state == ON) {
                    raiseGate();
                }
                m_remoteSensorActive = false;
            }
            break;
        case RAISEGATE:
            printf("%s: RAISEGATE\n", __PRETTY_FUNCTION__);
            raiseGate();
            break;
        case GATERAISED:
            printf("%s: GATERAISED\n", __PRETTY_FUNCTION__);
            turnOff();
            break;
        case LOWERGATE:
            printf("%s: LOWERGATE\n", __PRETTY_FUNCTION__);
            break;
        case GATELOWERED:
            printf("%s: GATELOWERED\n", __PRETTY_FUNCTION__);
            break;
    }
}