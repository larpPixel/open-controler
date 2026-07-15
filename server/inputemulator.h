#pragma once

#include <QObject>
#include <linux/uinput.h>
#include "protocol.h"

class InputEmulator : public QObject
{
    Q_OBJECT

public:
    explicit InputEmulator(QObject *parent = nullptr);
    ~InputEmulator();

    bool init();
    void processInput(const QRC::InputState &state);
    void destroy();

    bool isValid() const { return m_fd >= 0; }

private:
    void emitEvent(uint16_t type, uint16_t code, int32_t value);
    void emitSyn();
    void setupAbsAxis(int code, int min, int max, int flat, int fuzz);

    int m_fd = -1;
    struct uinput_setup m_usetup {};
};
