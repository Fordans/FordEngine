#include "FDE/Core/Log.hpp"
#include "FDE/Runtime/RuntimeApplication.hpp"
#include <string>

int main(int argc, char* argv[])
{
    if (argc < 2 || !argv[1])
    {
        FDE_LOG_CLIENT_ERROR("Usage: FordRuntime <path-to-.fproject>");
        return 1;
    }

    std::string path(argv[1]);
    FDE::RuntimeApplication app(path);
    app.Run();
    return 0;
}
