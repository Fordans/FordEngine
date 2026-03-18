#pragma once

#include "FDE/Export.hpp"
#include <string>

namespace FDE
{

class World;

/// Project identifier file (.fproject) descriptor.
/// .fproject must reside in the project root directory.
/// Format: JSON with required fields and optional metadata.
/// Supports embedding World-Scene-Object hierarchy for persistence.
struct FDE_API ProjectDescriptor
{
    /// Project display name
    std::string name;
    /// Project version (semver-like, e.g. "1.0.0")
    std::string version;
    /// Schema version for format evolution (currently 1)
    int schemaVersion = 1;

    /// Load from a directory. Looks for .fproject in the given path.
    /// \param projectRoot Absolute path to project root (directory containing .fproject)
    /// \param outDescriptor Populated on success
    /// \param outError Error message on failure
    /// \param outWorld Optional. If non-null, populated from "world" key in .fproject
    /// \return true if loaded successfully
    static bool LoadFromDirectory(const std::string& projectRoot, ProjectDescriptor& outDescriptor,
                                  std::string& outError, World* outWorld = nullptr);

    /// Load from a .fproject file path.
    /// \param fprojectPath Absolute path to .fproject file
    /// \param outDescriptor Populated on success
    /// \param outError Error message on failure
    /// \param outWorld Optional. If non-null, populated from "world" key in .fproject
    /// \return true if loaded successfully
    static bool LoadFromFile(const std::string& fprojectPath, ProjectDescriptor& outDescriptor,
                             std::string& outError, World* outWorld = nullptr);

    /// Save to project root. Creates/overwrites .fproject.
    /// \param projectRoot Absolute path to project root
    /// \param outError Error message on failure
    /// \param world Optional. If non-null, serialized into "world" key in .fproject
    /// \return true if saved successfully
    bool SaveToDirectory(const std::string& projectRoot, std::string& outError,
                         const World* world = nullptr) const;

    /// Check if a directory contains a valid .fproject.
    static bool IsProjectDirectory(const std::string& path);

    /// Get the canonical .fproject filename
    static const char* GetFileName() { return ".fproject"; }
};

} // namespace FDE
