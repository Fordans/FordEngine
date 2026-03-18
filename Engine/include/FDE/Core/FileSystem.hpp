#pragma once

#include "FDE/Export.hpp"
#include <string>

namespace FDE
{

/// Unified file system utilities: path resolution and project root management.
class FDE_API FileSystem
{
  public:
    /// Get the directory containing the executable (cross-platform).
    static std::string GetExecutableDirectory();

    /// Resolve engine resource path (icons, fonts, etc.).
    /// Search order: CWD/Resources/ -> exe/Resources/
    static std::string ResolveEngineResource(const std::string& relativePath);

    /// Resolve project resource path. Returns empty if no project is open.
    /// Returns absolute path: projectRoot/relativePath
    static std::string ResolveProjectPath(const std::string& relativePath);

    /// Project root directory. Empty string = no project.
    static void SetProjectRoot(const std::string& path);
    static std::string GetProjectRoot();
    static bool HasProject();
};

} // namespace FDE
