#ifndef GAMEPAD_H
#define GAMEPAD_H

#include "common.h"

enum class Button
{
    A = 0, B = 1, SELECT = 2, START = 3, UP = 4, DOWN = 5, LEFT = 6, RIGHT = 7
};

class Bus;

class Gamepad
{
public:
    void setBus(Bus *pBus)
    {
        m_pBus = pBus;
    }

    /*!
     * @param b Button ID
     * @param pressed true if pressed, false if released
     * @param turbo emulate 6 press / release actions per second while pressed
     * @param pad2 action from the second pad if it is doubled
     */
    void buttonEvent(Button b, bool pressed, bool turbo, bool pad2) noexcept;

    void lightGunDetector(bool on) noexcept
    {
        m_lightGunDetector = on;
    }

    void lightGunTrigger(bool pressed) noexcept
    {
        m_lightGunTrigger = pressed;
    }

    void strobe() noexcept
    {
        m_ind = 0;
    }

    c6502_byte_t readRegister() noexcept;

private:
    Bus *m_pBus = nullptr;

    bool m_buttonState[16] = { };

    // "Turbo" buttons emulation
    int m_pressTime[16] = { };

    bool m_lightGunDetector = false,
         m_lightGunTrigger = false;

    int m_ind = 0;

    bool turboTest(int btnInd) const noexcept;
};

#endif
