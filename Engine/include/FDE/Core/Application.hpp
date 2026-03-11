#pragma once

#include "FDE/Export.hpp"

namespace FDE {

class FDE_API Application {
public:
    Application() = default;
    virtual ~Application() = default;

    void Run();
};

} // namespace FDE
