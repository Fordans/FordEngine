#include "FDE/pch.hpp"
#include "FDE/Asset/AssetType.hpp"
#include <cstring>

namespace FDE
{

const char* AssetTypeToString(AssetType t)
{
    switch (t)
    {
    case AssetType::Texture2D:
        return "Texture2D";
    case AssetType::Shader:
        return "Shader";
    case AssetType::Mesh2D:
        return "Mesh2D";
    case AssetType::Mesh3D:
        return "Mesh3D";
    default:
        return "Unknown";
    }
}

AssetType AssetTypeFromString(std::string_view s)
{
    if (s == "Texture2D")
        return AssetType::Texture2D;
    if (s == "Shader")
        return AssetType::Shader;
    if (s == "Mesh2D")
        return AssetType::Mesh2D;
    if (s == "Mesh3D")
        return AssetType::Mesh3D;
    return AssetType::Unknown;
}

} // namespace FDE
