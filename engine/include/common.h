/* Generic 6502 definitions
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef uint8_t c6502_byte_t;
typedef uint16_t c6502_word_t;
typedef uint32_t c6502_d_word_t;

// This signed type is used for comparisions in arithmetic operations.
typedef int16_t c6502_test_t;

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