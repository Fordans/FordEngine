#include "FDE/Core/Application.hpp"
#include "FDE/Core/Log.hpp"
#include <memory>

int main(int argc, char* argv[]) {
    FDE::LogInfo("FDEditor starting...");

    auto app = std::make_unique<FDE::Application>();
    app->Run();

    FDE::LogInfo("FDEditor exiting.");
    return 0;
}
