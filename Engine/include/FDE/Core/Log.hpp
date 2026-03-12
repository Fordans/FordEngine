#pragma once

#include "FDE/Export.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace FDE
{

inline spdlog::logger* GetCoreLogger()
{
    static std::shared_ptr<spdlog::logger> s_logger = []()
    {
        auto logger = spdlog::stdout_color_mt("FDE");
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [FDE] %v");
        return logger;
    }();
    return s_logger.get();
}

inline spdlog::logger* GetClientLogger()
{
    static std::shared_ptr<spdlog::logger> s_logger = []()
    {
        auto logger = spdlog::stdout_color_mt("Client");
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [Client] %v");
        return logger;
    }();
    return s_logger.get();
}

template <typename... Args> inline void LogCoreTrace(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetCoreLogger()->trace(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogCoreDebug(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetCoreLogger()->debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogCoreInfo(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetCoreLogger()->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogCoreWarning(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetCoreLogger()->warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogCoreError(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetCoreLogger()->error(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogCoreCritical(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetCoreLogger()->critical(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogClientTrace(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetClientLogger()->trace(fmt, std::forward<Args>(args)...);
}
template <typename... Args> inline void LogClientDebug(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetClientLogger()->debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogClientInfo(fmt::format_string<Args...> fmt, Args&&... args)  
{
    GetClientLogger()->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogClientWarning(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetClientLogger()->warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogClientError(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetClientLogger()->error(fmt, std::forward<Args>(args)...);
}

template <typename... Args> inline void LogClientCritical(fmt::format_string<Args...> fmt, Args&&... args)
{
    GetClientLogger()->critical(fmt, std::forward<Args>(args)...);
}

FDE_API void SetLogCoreLevel(spdlog::level::level_enum level);
FDE_API void SetLogClientLevel(spdlog::level::level_enum level);

} // namespace FDE

#ifdef _DEBUG
#define FDE_ENABLE_LOGGING
#endif

// Macros for easy logging
#ifdef FDE_ENABLE_LOGGING
#define FDE_LOG_CORE_TRACE(...) ::FDE::LogCoreTrace(__VA_ARGS__)
#define FDE_LOG_CORE_DEBUG(...) ::FDE::LogCoreDebug(__VA_ARGS__)
#define FDE_LOG_CORE_INFO(...) ::FDE::LogCoreInfo(__VA_ARGS__)
#define FDE_LOG_CORE_WARN(...) ::FDE::LogCoreWarning(__VA_ARGS__)
#define FDE_LOG_CORE_ERROR(...) ::FDE::LogCoreError(__VA_ARGS__)
#define FDE_LOG_CORE_CRITICAL(...) ::FDE::LogCoreCritical(__VA_ARGS__)
#define FDE_LOG_CLIENT_TRACE(...) ::FDE::LogClientTrace(__VA_ARGS__)
#define FDE_LOG_CLIENT_DEBUG(...) ::FDE::LogClientDebug(__VA_ARGS__)
#define FDE_LOG_CLIENT_INFO(...) ::FDE::LogClientInfo(__VA_ARGS__)
#define FDE_LOG_CLIENT_WARN(...) ::FDE::LogClientWarning(__VA_ARGS__)
#define FDE_LOG_CLIENT_ERROR(...) ::FDE::LogClientError(__VA_ARGS__)
#define FDE_LOG_CLIENT_CRITICAL(...) ::FDE::LogClientCritical(__VA_ARGS__)
#else
#define FDE_LOG_CORE_TRACE(...)
#define FDE_LOG_CORE_DEBUG(...)
#define FDE_LOG_CORE_INFO(...)
#define FDE_LOG_CORE_WARN(...)
#define FDE_LOG_CORE_ERROR(...)
#define FDE_LOG_CORE_CRITICAL(...)
#define FDE_LOG_CLIENT_TRACE(...)
#define FDE_LOG_CLIENT_DEBUG(...)
#define FDE_LOG_CLIENT_INFO(...)
#define FDE_LOG_CLIENT_WARN(...)
#define FDE_LOG_CLIENT_ERROR(...)
#define FDE_LOG_CLIENT_CRITICAL(...)
#endif
