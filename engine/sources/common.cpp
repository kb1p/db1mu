#include "common.h"
#include <new>
#include <cstring>

Exception::Exception(Exception::Code code, const char* msg):
    m_code(code)
{
    if (msg)
    {
        m_msg = new(std::nothrow) char[strlen(msg) + 1];
        if (m_msg)
            strcpy(m_msg, msg);
    }
}

Exception::~Exception()
{
    delete[] m_msg;
}

