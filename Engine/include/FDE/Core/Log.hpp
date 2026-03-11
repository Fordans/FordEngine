#pragma once

#include "FDE/Export.hpp"
#include <string>

namespace FDE {

FDE_API void LogInfo(const std::string& message);
FDE_API void LogWarning(const std::string& message);
FDE_API void LogError(const std::string& message);

} // namespace FDE
