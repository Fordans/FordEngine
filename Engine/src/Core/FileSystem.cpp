#include "FDE/pch.hpp"
#include "FDE/Core/FileSystem.hpp"

#include <filesystem>
#include <vector>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

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
    uint32_t bufsize = 0;
    _NSGetExecutablePath(nullptr, &bufsize);
    if (bufsize == 0)
        return {};
    std::vector<char> buf(bufsize);
    if (_NSGetExecutablePath(buf.data(), &bufsize) != 0)
        return {};
    std::filesystem::path exe(buf.data());
    return exe.parent_path().string();
#elif defined(__linux__)
    std::vector<char> buf(4096);
    const ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (len <= 0)
        return {};
    buf[static_cast<size_t>(len)] = '\0';
    std::filesystem::path exe(buf.data());
    return exe.parent_path().string();
#else
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
