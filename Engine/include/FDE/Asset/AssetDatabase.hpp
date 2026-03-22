#pragma once

#include "FDE/Asset/AssetRegistry.hpp"
#include "FDE/Export.hpp"
#include <string>

namespace FDE
{

/// Editor-side import / scan / cook into Library/Built and AssetRegistry.json
class FDE_API AssetDatabase
{
  public:
    /// Create `Assets/`, `Library/`, `Library/Built/` if missing.
    static bool EnsureLibraryLayout(const std::string& projectRoot, std::string& outError);

    /// Walk `Assets/` and register or update entries; copies/cooks into Built.
    static bool RescanAssets(const std::string& projectRoot, std::string& outError);

    /// Import a single file under project `Assets/` (must exist).
    static bool ImportAssetFile(const std::string& projectRoot, const std::string& assetFileAbsolute,
                                std::string& outError);

    /// Pack all Built blobs into `Build/<name>.fdepack` (uses [AssetPack](AssetPack.hpp)).
    static bool BuildFdepack(const std::string& projectRoot, const std::string& outputRelativePath,
                             std::string& outError);
};

} // namespace FDE
