#include "common.h"
#include <new>
#include <cstring>

Exception::Exception(Exception::Code code, const char *msg):
    m_code(code)
{
    if (msg)
        m_msg.assign(msg);
    else
        m_msg.clear();
}
