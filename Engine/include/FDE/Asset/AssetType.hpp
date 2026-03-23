#pragma once

#include "FDE/Export.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace FDE
{

/// Engine asset kinds (import / cook / runtime).
enum class AssetType : uint8_t
{
    Unknown = 0,
    Texture2D,
    Shader,
    Mesh2D,
    Mesh3D,
};

FDE_API const char* AssetTypeToString(AssetType t);
FDE_API AssetType AssetTypeFromString(std::string_view s);

} // namespace FDE
