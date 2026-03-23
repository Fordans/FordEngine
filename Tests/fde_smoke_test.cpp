#include <FDE/Core/FileSystem.hpp>
#include <FDE/Project/ProjectDescriptor.hpp>
#include <cstdlib>
#include <string>

int main()
{
#if defined(_WIN32) || defined(__linux__)
    if (FDE::FileSystem::GetExecutableDirectory().empty())
        return 1;
#elif defined(__APPLE__)
    if (FDE::FileSystem::GetExecutableDirectory().empty())
        return 1;
#endif

    FDE::ProjectDescriptor desc;
    std::string err;
    if (!FDE::ProjectDescriptor::LoadFromFile("___nonexistent___.fproject", desc, err))
    {
        if (err.empty())
            return 2;
    }

    return 0;
}
