#include "FDE/pch.hpp"
#include "FDE/Core/FileSystem.hpp"

namespace FDE
{

namespace
{
std::string s_projectRoot;
}

std::string FileSystem::GetExecutableDirectory()
{
#if defined(_WIN32)
    char path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) == 0)
        return {};
    std::filesystem::path p(path);
    return p.parent_path().string();
#elif defined(__APPLE__)
    // TODO: implement for macOS (NSBundle)
    return {};
#else
    // TODO: implement for Linux (/proc/self/exe)
    return {};
#endif
}

std::string FileSystem::ResolveEngineResource(const std::string& relativePath)
{
    namespace fs = std::filesystem;
    std::string cwdPath = "Resources/" + relativePath;
    if (fs::exists(cwdPath))
        return fs::absolute(cwdPath).string();
    std::string exeDir = GetExecutableDirectory();
    if (!exeDir.empty())
    {
        std::string exePath = exeDir + "/Resources/" + relativePath;
        if (fs::exists(exePath))
            return exePath;
    }
    return {};
}

std::string FileSystem::ResolveProjectPath(const std::string& relativePath)
{
    if (s_projectRoot.empty())
        return {};
    namespace fs = std::filesystem;
    fs::path fullPath = fs::path(s_projectRoot) / relativePath;
    return fullPath.lexically_normal().string();
}

void FileSystem::SetProjectRoot(const std::string& path)
{
    s_projectRoot = path;
}

std::string FileSystem::GetProjectRoot()
{
    return s_projectRoot;
}

bool FileSystem::HasProject()
{
    return !s_projectRoot.empty();
}

} // namespace FDE
