#include "FDE/Editor/EditorApplication.hpp"
#include <string>

int main(int argc, char* argv[])
{
    std::string initialProjectPath;
    if (argc >= 2 && argv[1])
    {
        std::string arg(argv[1]);
        if (arg.size() >= 9 && arg.compare(arg.size() - 9, 9, ".fproject") == 0)
            initialProjectPath = arg;
    }

    FDE::EditorApplication app(initialProjectPath);
    app.Run();
    return 0;
}
