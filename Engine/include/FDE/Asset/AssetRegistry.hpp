#pragma once

#include "FDE/Asset/AssetId.hpp"
#include "FDE/Asset/AssetType.hpp"
#include "FDE/Export.hpp"
#include <string>
#include <vector>

namespace FDE
{

/// One row in Library/AssetRegistry.json
struct FDE_API AssetRecord
{
    AssetId guid;
    /// e.g. Assets/Textures/foo.png
    std::string logicalPath;
    AssetType type = AssetType::Unknown;
    /// FNV-1a hex of source file bytes (MVP).
    std::string contentHash;
    /// Relative to project root: Library/Built/...
    std::string builtRelativePath;
    std::vector<AssetId> dependencies;
};

/// Central catalog for a project (JSON on disk).
class FDE_API AssetRegistry
{
  public:
    int schemaVersion = 1;
    std::vector<AssetRecord> assets;

    /// Load from Library/AssetRegistry.json under project root.
    static bool LoadFromProject(const std::string& projectRoot, AssetRegistry& out, std::string& outError);

    /// Save to Library/AssetRegistry.json
    bool SaveToProject(const std::string& projectRoot, std::string& outError) const;

    const AssetRecord* FindByGuid(const AssetId& id) const;
    const AssetRecord* FindByLogicalPath(std::string_view logicalPath) const;

    void Upsert(AssetRecord record);
};

} // namespace FDE
