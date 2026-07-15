#include "inputemulator.h"

#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <linux/input.h>

InputEmulator::InputEmulator(QObject *parent)
    : QObject(parent)
{
}

InputEmulator::~InputEmulator()
{
    destroy();
}

void InputEmulator::setupAbsAxis(int code, int min, int max, int flat, int fuzz)
{
    struct uinput_abs_setup absSetup {};
    absSetup.code = code;
    absSetup.absinfo.minimum = min;
    absSetup.absinfo.maximum = max;
    absSetup.absinfo.flat = flat;
    absSetup.absinfo.fuzz = fuzz;
    absSetup.absinfo.value = 0;
    ioctl(m_fd, UI_ABS_SETUP, &absSetup);
}

bool InputEmulator::init()
{
    m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        qWarning() << "InputEmulator: Failed to open /dev/uinput:" << strerror(errno);
        return false;
    }

    // Enable event types
    ioctl(m_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(m_fd, UI_SET_EVBIT, EV_KEY);

    // --- Absolute axes ---

    // Sticks: [-32767, 32767]
    ioctl(m_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(m_fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(m_fd, UI_SET_ABSBIT, ABS_RX);
    ioctl(m_fd, UI_SET_ABSBIT, ABS_RY);
    setupAbsAxis(ABS_X,  -32767, 32767, 512, 16);
    setupAbsAxis(ABS_Y,  -32767, 32767, 512, 16);
    setupAbsAxis(ABS_RX, -32767, 32767, 512, 16);
    setupAbsAxis(ABS_RY, -32767, 32767, 512, 16);

    // Triggers: [0, 255]
    ioctl(m_fd, UI_SET_ABSBIT, ABS_Z);
    ioctl(m_fd, UI_SET_ABSBIT, ABS_RZ);
    setupAbsAxis(ABS_Z,  0, 255, 0, 0);
    setupAbsAxis(ABS_RZ, 0, 255, 0, 0);

    // D-pad HAT: [-1, 1]
    ioctl(m_fd, UI_SET_ABSBIT, ABS_HAT0X);
    ioctl(m_fd, UI_SET_ABSBIT, ABS_HAT0Y);
    setupAbsAxis(ABS_HAT0X, -1, 1, 0, 0);
    setupAbsAxis(ABS_HAT0Y, -1, 1, 0, 0);

    // Motion: tilt axes [-32767, 32767]
    ioctl(m_fd, UI_SET_ABSBIT, ABS_TILT_X);
    ioctl(m_fd, UI_SET_ABSBIT, ABS_TILT_Y);
    setupAbsAxis(ABS_TILT_X, -32767, 32767, 0, 0);
    setupAbsAxis(ABS_TILT_Y, -32767, 32767, 0, 0);

    // --- Buttons ---
    ioctl(m_fd, UI_SET_KEYBIT, BTN_A);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_B);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_X);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_Y);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_TL);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_TR);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_START);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_THUMBL);
    ioctl(m_fd, UI_SET_KEYBIT, BTN_THUMBR);

    // Create the device
    std::memset(&m_usetup, 0, sizeof(m_usetup));
    m_usetup.id.bustype = BUS_USB;
    m_usetup.id.vendor  = 0x045e;  // Mimic Xbox-like vendor
    m_usetup.id.product = 0x0b13;
    m_usetup.id.version = 1;
    std::strncpy(m_usetup.name, "QR Controller Virtual Gamepad", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(m_fd, UI_DEV_SETUP, &m_usetup) < 0) {
        qWarning() << "InputEmulator: UI_DEV_SETUP failed:" << strerror(errno);
        close(m_fd);
        m_fd = -1;
        return false;
    }

    if (ioctl(m_fd, UI_DEV_CREATE) < 0) {
        qWarning() << "InputEmulator: UI_DEV_CREATE failed:" << strerror(errno);
        close(m_fd);
        m_fd = -1;
        return false;
    }

    qInfo() << "InputEmulator: Virtual gamepad created successfully";
    return true;
}

void InputEmulator::destroy()
{
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        close(m_fd);
        m_fd = -1;
        qInfo() << "InputEmulator: Virtual gamepad destroyed";
    }
}

void InputEmulator::emitEvent(uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev {};
    ev.type  = type;
    ev.code  = code;
    ev.value = value;
    if (write(m_fd, &ev, sizeof(ev)) < 0) {
        // Silently ignore write failures (non-blocking)
    }
}

void InputEmulator::emitSyn()
{
    emitEvent(EV_SYN, SYN_REPORT, 0);
}

void InputEmulator::processInput(const QRC::InputState &state)
{
    if (m_fd < 0)
        return;

    bool changed = false;

    // Joysticks
    if (state.leftStickX != m_lastState.leftStickX) {
        emitEvent(EV_ABS, ABS_X, state.leftStickX);
        changed = true;
    }
    if (state.leftStickY != m_lastState.leftStickY) {
        emitEvent(EV_ABS, ABS_Y, state.leftStickY);
        changed = true;
    }
    if (state.rightStickX != m_lastState.rightStickX) {
        emitEvent(EV_ABS, ABS_RX, state.rightStickX);
        changed = true;
    }
    if (state.rightStickY != m_lastState.rightStickY) {
        emitEvent(EV_ABS, ABS_RY, state.rightStickY);
        changed = true;
    }

    // Triggers
    if (state.leftTrigger != m_lastState.leftTrigger) {
        emitEvent(EV_ABS, ABS_Z, state.leftTrigger);
        changed = true;
    }
    if (state.rightTrigger != m_lastState.rightTrigger) {
        emitEvent(EV_ABS, ABS_RZ, state.rightTrigger);
        changed = true;
    }

    // D-pad via HAT
    int hatX = 0, hatY = 0;
    if (state.buttons & QRC::Btn::Left)  hatX = -1;
    if (state.buttons & QRC::Btn::Right) hatX =  1;
    if (state.buttons & QRC::Btn::Up)    hatY = -1;
    if (state.buttons & QRC::Btn::Down)  hatY =  1;

    int lastHatX = 0, lastHatY = 0;
    if (m_lastState.buttons & QRC::Btn::Left)  lastHatX = -1;
    if (m_lastState.buttons & QRC::Btn::Right) lastHatX =  1;
    if (m_lastState.buttons & QRC::Btn::Up)    lastHatY = -1;
    if (m_lastState.buttons & QRC::Btn::Down)  lastHatY =  1;

    if (hatX != lastHatX) {
        emitEvent(EV_ABS, ABS_HAT0X, hatX);
        changed = true;
    }
    if (hatY != lastHatY) {
        emitEvent(EV_ABS, ABS_HAT0Y, hatY);
        changed = true;
    }

    // Buttons
    auto checkButton = [&](uint16_t mask, uint16_t keyBtn) {
        bool curr = (state.buttons & mask);
        bool prev = (m_lastState.buttons & mask);
        if (curr != prev) {
            emitEvent(EV_KEY, keyBtn, curr ? 1 : 0);
            changed = true;
        }
    };

    checkButton(QRC::Btn::A, BTN_A);
    checkButton(QRC::Btn::B, BTN_B);
    checkButton(QRC::Btn::X, BTN_X);
    checkButton(QRC::Btn::Y, BTN_Y);
    checkButton(QRC::Btn::LB, BTN_TL);
    checkButton(QRC::Btn::RB, BTN_TR);
    checkButton(QRC::Btn::Start, BTN_START);
    checkButton(QRC::Btn::Select, BTN_SELECT);
    checkButton(QRC::Btn::L3, BTN_THUMBL);
    checkButton(QRC::Btn::R3, BTN_THUMBR);

    // Motion
    if (state.motionEnabled & QRC::Motion::AccelOnly) {
        if (state.accelX != m_lastState.accelX) {
            emitEvent(EV_ABS, ABS_TILT_X, state.accelX);
            changed = true;
        }
        if (state.accelY != m_lastState.accelY) {
            emitEvent(EV_ABS, ABS_TILT_Y, state.accelY);
            changed = true;
        }
    }

    if (changed) {
        emitSyn();
    }

    m_lastState = state;
}
