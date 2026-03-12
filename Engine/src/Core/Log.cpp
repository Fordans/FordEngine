#include "FDE/Core/Log.hpp"

namespace FDE
{

void SetLogCoreLevel(spdlog::level::level_enum level)
{
    GetCoreLogger()->set_level(level);
}

void SetLogClientLevel(spdlog::level::level_enum level)
{
    GetClientLogger()->set_level(level);
}

} // namespace FDE
