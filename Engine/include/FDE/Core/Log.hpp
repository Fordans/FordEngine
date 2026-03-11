#pragma once

#include "FDE/Export.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace FDE
{

inline spdlog::logger* GetLogger()
{
    static std::shared_ptr<spdlog::logger> s_logger = []()
    {
        auto logger = spdlog::stdout_color_mt("FDE");
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [FDE] %v");
        return logger;
    }();
    return s_logger.get();
}

template <typename... Args> inline void LogTrace(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetLogger()->trace(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogDebug(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetLogger()->debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogInfo(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetLogger()->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogWarning(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetLogger()->warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogError(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetLogger()->error(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogCritical(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetLogger()->critical(fmt, std::forward<Args>(args)...);
}

FDE_API void SetLogLevel(spdlog::level::level_enum level);

} // namespace FDE
