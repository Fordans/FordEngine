#include "FDE/Core/Log.hpp"
#include <iostream>

namespace FDE {

void LogInfo(const std::string& message) {
    std::cout << "[FDE Info] " << message << std::endl;
}

void LogWarning(const std::string& message) {
    std::cerr << "[FDE Warning] " << message << std::endl;
}

void LogError(const std::string& message) {
    std::cerr << "[FDE Error] " << message << std::endl;
}

} // namespace FDE
