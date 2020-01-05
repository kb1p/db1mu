#include "gamepad.h"
#include "bus.h"

#include <algorithm>

// Repeat press / release sequence at 10 Hz frequency
static constexpr int TURBO_FREQ = 20;

void Gamepad::buttonEvent(Button b, bool pressed, bool turbo, bool pad2) noexcept
{
    int i = static_cast<int>(b);
    assert(i >= 0 && i < 8);
    if (pad2)
        i += 8;

    m_buttonState[i] = pressed;
    m_turboOn[i] = turbo;
}

bool Gamepad::turboTest(int btnInd) const noexcept
{
    if (!m_turboOn[btnInd])
        return true;

    return divrnd(bus().currentTimeMs() * TURBO_FREQ, 1000) % 2 == 0;
}

c6502_byte_t Gamepad::readRegister() noexcept
{
    constexpr int IND_LIM = static_cast<int>(sizeof(m_buttonState) / sizeof(m_buttonState[0]));

    const int ind = std::min(m_ind++, IND_LIM);
    c6502_byte_t v = 0u;
    if (m_buttonState[ind] && turboTest(ind))
        v = 1u;

    if (m_lightGunTrigger)
        v |= 0b100u;
    if (!m_lightGunDetector)
        v |= 0b1000u;

    return v;
}
