#include "gamepad.h"

#include <algorithm>
#include <cassert>

void Gamepad::buttonEvent(Button b, bool pressed, bool turbo, bool pad2) noexcept
{
    const int i = static_cast<int>(b);
    assert(i >= 0 && i < 8);

    m_buttonState[pad2 ? i + 8 : i] = pressed;
}

c6502_byte_t Gamepad::readRegister() noexcept
{
    constexpr int IND_LIM = static_cast<int>(sizeof(m_buttonState) / sizeof(m_buttonState[0]));

    c6502_byte_t v = m_buttonState[std::min(m_ind++, IND_LIM)] ? 1u : 0u;

    if (m_lightGunTrigger)
        v |= 0b100u;
    if (!m_lightGunDetector)
        v |= 0b1000u;

    return v;
}
