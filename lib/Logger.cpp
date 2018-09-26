#include "Logger.h"

namespace vazgen {

namespace {

spdlog::level::level_enum get_spdlog_level(Logger::Level level)
{
    switch (level) {
    case Logger::INFO:
        return spdlog::level::level_enum::info;
    case Logger::WARN:
        return spdlog::level::level_enum::warn;
    case Logger::ERR:
        return spdlog::level::level_enum::err;
    case Logger::DEBUG:
        return spdlog::level::level_enum::debug;
    default:
        return spdlog::level::level_enum::off;
    };
    return spdlog::level::level_enum::off;
}

}

Logger::Logger(const std::string& name)
{
    m_logger = spdlog::get(name);
}

void Logger::setLevel(Level level)
{
    m_logger->set_level(get_spdlog_level(level));
}

void Logger::info(const std::string& msg)
{
    m_logger->info(msg);
}

void Logger::warn(const std::string& msg)
{
    m_logger->warn(msg);
}

void Logger::error(const std::string& msg)
{
    m_logger->error(msg);
}

void Logger::debug(const std::string& msg)
{
    m_logger->debug(msg);
}

} // namespace vazgen

