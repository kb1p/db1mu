#include "gamepad.h"

#include <algorithm>
#include <cassert>

using namespace std::chrono;

static const system_clock::time_point ZERO_TIME;

// ~6 on/off switches per second
static const milliseconds TURBO_INTERVAL { 83 };

void Gamepad::buttonEvent(Button b, bool pressed, bool turbo, bool pad2) noexcept
{
    int i = static_cast<int>(b);
    assert(i >= 0 && i < 8);
    if (pad2)
        i += 8;

    m_buttonState[i] = pressed;

    if (turbo)
        m_pressTime[i] = system_clock::now();
    else
        m_pressTime[i] = ZERO_TIME;
}

inline bool turboOn(const system_clock::time_point &p) noexcept
{
    return duration_cast<milliseconds>(system_clock::now() - p) / TURBO_INTERVAL % 2 == 0;
}

c6502_byte_t Gamepad::readRegister() noexcept
{
    constexpr int IND_LIM = static_cast<int>(sizeof(m_buttonState) / sizeof(m_buttonState[0]));

    const int ind = std::min(m_ind++, IND_LIM);
    c6502_byte_t v = 0u;
    if (m_buttonState[ind] &&
        (m_pressTime[ind] == ZERO_TIME || turboOn(m_pressTime[ind])))
        v = 1u;

    if (m_lightGunTrigger)
        v |= 0b100u;
    if (!m_lightGunDetector)
        v |= 0b1000u;

    return v;
}
