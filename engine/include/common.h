/* Generic 6502 definitions
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <iostream>
#include <cassert>
#include <string>

typedef unsigned int uint;
typedef uint8_t c6502_byte_t;
typedef uint16_t c6502_word_t;
typedef uint32_t c6502_d_word_t;

inline constexpr c6502_byte_t hi_byte(c6502_word_t x) noexcept
{
    return static_cast<c6502_byte_t>((x >> 8) & 0xFFu);
}

inline constexpr c6502_byte_t lo_byte(c6502_word_t x) noexcept
{
    return static_cast<c6502_byte_t>(x & 0xFFu);
}

inline constexpr c6502_word_t combine(c6502_byte_t lo, c6502_byte_t hi) noexcept
{
    return (static_cast<c6502_word_t>(hi & 0xFFu) << 8) | (lo & 0xFFu);
}

inline constexpr int divrnd(int a, int b) noexcept
{
    return (a + b / 2) / b;
}

class Exception
{
public:
    enum Code
    {
        Unknown,
        IOFailure,
        IllegalFormat,
        SizeOverflow,
        IllegalArgument,
        IllegalOperation
    };

    Exception(Code code, const char *msg = nullptr);

    Code code() const
    {
        return m_code;
    }

    const char *message() const
    {
        return m_msg.c_str();
    }

private:
    Code m_code;
    std::string m_msg;
};

class Bus;

class Component
{
    Bus *m_pBus = nullptr;

protected:
    friend class Bus;

    void setBus(Bus *pBus) noexcept
    {
        m_pBus = pBus;
    }

    Bus &bus() const noexcept
    {
        assert(m_pBus != nullptr && "component is not attached to the Bus");
        return *m_pBus;
    }

public:
    virtual size_t saveState(std::ostream&)
    {
        assert(false && "save is not implemented");
        return 0;
    }

    virtual size_t loadState(std::istream&)
    {
        assert(false && "load is not implemented");
        return 0;
    }
};

template <typename T>
class Maybe
{
    T m_data;
    bool m_isNothing = true;

public:
    Maybe() noexcept = default;
    Maybe(T &&v) noexcept:
        m_data { std::forward<T>(v) },
        m_isNothing { false }
    {
    }
    Maybe(const Maybe&) noexcept = default;
    Maybe(Maybe&&) noexcept = default;

    Maybe &operator=(const Maybe &m) noexcept
    {
        m_isNothing = m.m_isNothing;
        if (!m_isNothing)
            m_data = m.m_data;

        return *this;
    }

    Maybe &operator=(Maybe &&m) noexcept
    {
        m_isNothing = m.m_isNothing;
        if (!m_isNothing)
            m_data = std::move(m.m_data);

        return *this;
    }

    Maybe &operator=(T v) noexcept
    {
        m_isNothing = false;
        m_data = std::move(v);

        return *this;
    }

    bool isNothing() const noexcept
    {
        return m_isNothing;
    }

    const T &value(const T &def) const noexcept
    {
        return m_isNothing ? def : m_data;
    }

    const T &value() const
    {
        if (m_isNothing)
            throw Exception { Exception::IllegalOperation,
                              "attempt to get value of nothing" };

        return m_data;
    }

    void set(T &&v) noexcept
    {
        m_data = std::forward<T>(v);
    }

    void unset() noexcept
    {
        m_isNothing = true;
    }

    bool operator==(const Maybe &x) const noexcept
    {
        return m_isNothing && x.m_isNothing ? true :
               !m_isNothing && !x.m_isNothing ? m_data == x.m_data :
               false;
    }

    bool operator!=(const Maybe &x) const noexcept
    {
        return !(*this == x);
    }
};

#endif
