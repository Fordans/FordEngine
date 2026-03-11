#include "FDE/Core/Log.hpp"

namespace FDE
{

void SetLogLevel(spdlog::level::level_enum level)
{
    GetLogger()->set_level(level);
}

} // namespace FDE
