#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <string>
#include <chrono>

class Log
{
public:
    enum Severity: uint8_t
    {
        LVL_ERROR = 1u,
        LVL_WARNING = 2u,
        LVL_INFO = 4u,
        LVL_DEBUG = 8u,
        LVL_VERBOSE = 16u,
    };

    using Filter = uint8_t;
    static constexpr Filter LEVEL_SILENT = LVL_ERROR,
                            LEVEL_STD = LVL_ERROR | LVL_WARNING | LVL_INFO,
                            LEVEL_DEBUG = LEVEL_STD | LVL_DEBUG,
                            LEVEL_VERBOSE = LEVEL_DEBUG | LVL_VERBOSE;

    struct Config
    {
        std::ostream *pOutput;
        Filter filter;
        bool printTime,
             printSeverity,
             autoFlush;
        std::string fieldSep;
    };

    Config &config() noexcept
    {
        return m_config;
    }

    void print(Severity sl, const char *msg, ...);
    
    static Log &instance() noexcept
    {
        if (!s_pInst)
            s_pInst = new Log;
        return *s_pInst;
    }

    template <typename... Args>
    static void e(Args&&... a) noexcept
    {
        instance().print(LVL_ERROR, std::forward<decltype(a)>(a)...);
    }

    template <typename... Args>
    static void w(Args&&... a) noexcept
    {
        instance().print(LVL_WARNING, std::forward<decltype(a)>(a)...);
    }

    template <typename... Args>
    static void i(Args&&... a) noexcept
    {
        instance().print(LVL_INFO, std::forward<decltype(a)>(a)...);
    }

    template <typename... Args>
    static void d(Args&&... a) noexcept
    {
        instance().print(LVL_DEBUG, std::forward<decltype(a)>(a)...);
    }

    template <typename... Args>
    static void v(Args&&... a) noexcept
    {
        instance().print(LVL_VERBOSE, std::forward<decltype(a)>(a)...);
    }

private:
    static Log *s_pInst;

    Config m_config = {
        &std::cout,
        LEVEL_STD,
        true,
        true,
        false,
        "\t"
    };

    Log() = default;
};

#endif
