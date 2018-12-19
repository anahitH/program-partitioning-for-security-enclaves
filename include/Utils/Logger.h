#pragma once

#include "spdlog/spdlog.h"

namespace vazgen {

class Logger
{
public:
    enum Level {
        INFO = 0,
        WARN = 1,
        ERR = 2,
        DEBUG = 3
    };

public:
    Logger(const std::string& name);

    Logger (const Logger&) = delete;
    Logger (Logger&&) = delete;
    Logger& operator =(const Logger&) = delete;
    Logger& operator =(Logger&& ) = delete;

public:
    void setLevel(Level level);

    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void debug(const std::string& msg);

private:
    std::shared_ptr<spdlog::logger> m_logger;
}; // class Logger

} // namespace vazgen

