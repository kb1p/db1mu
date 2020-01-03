/* Generic 6502 definitions
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

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
    virtual ~Exception();

    Code code() const
    {
        return m_code;
    }

    const char *message() const
    {
        return m_msg;
    }

private:
    Code m_code;
    char *m_msg = nullptr;
};

#endif
