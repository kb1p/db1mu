#include "log.h"
#include <cstdio>
#include <cstdarg>
#include <cassert>

Log *Log::s_pInst = nullptr;

void Log::print(Severity sl, const char *fmt, ...)
{
    if ((m_config.filter & sl) == 0)
        return;

    assert(m_config.pOutput != nullptr && m_config.pOutput->good());
    constexpr int BUF_MAX = 2048;
    char buf[BUF_MAX];
    if (m_config.printTime)
    {
        using std::chrono::system_clock;
        auto t = system_clock::to_time_t(system_clock::now());
        auto lt = std::localtime(&t);
        snprintf(buf,
                 BUF_MAX,
                 "[%02d.%02d.%d %02d:%02d:%02d]",
                 lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900,
                 lt->tm_hour, lt->tm_min, lt->tm_sec);
        *m_config.pOutput << buf << m_config.fieldSep;
    }
    if (m_config.printSeverity)
    {
        const char *pSeverity = "???";
        switch (sl)
        {
            case LVL_ERROR:
                pSeverity = "ERROR";
                break;
            case LVL_WARNING:
                pSeverity = "WARNING";
                break;
            case LVL_INFO:
                pSeverity = "INFO";
                break;
            case LVL_DEBUG:
                pSeverity = "DEBUG";
                break;
            case LVL_VERBOSE:
                pSeverity = "VERBOSE";
        }
        *m_config.pOutput << pSeverity << m_config.fieldSep;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, BUF_MAX, fmt, args);
    va_end(args);
    *m_config.pOutput << buf << std::endl;
    if (m_config.autoFlush)
        m_config.pOutput->flush();
}
