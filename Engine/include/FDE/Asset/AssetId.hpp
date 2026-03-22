#pragma once

#include "FDE/Export.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace FDE
{

/// 128-bit GUID as canonical string: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
class FDE_API AssetId
{
  public:
    AssetId() = default;
    explicit AssetId(std::string id);

    /// RFC-4122-style random UUID (version 4).
    static AssetId Generate();

    /// Parse `guid:xxxxxxxx-...`, `fde://guid/xxxxxxxx-...`, or bare UUID string.
    static std::optional<AssetId> Parse(std::string_view ref);

    bool IsValid() const;
    const std::string& str() const { return m_id; }

    bool operator==(const AssetId& o) const { return m_id == o.m_id; }
    bool operator!=(const AssetId& o) const { return m_id != o.m_id; }

    /// True if `meshAsset` / path string refers to a GUID-based asset reference.
    static bool LooksLikeGuidReference(std::string_view meshAsset);

  private:
    std::string m_id;
};

/// Normalize logical asset path: forward slashes, optional `Assets/` prefix handling.
FDE_API std::string NormalizeLogicalPath(std::string_view path);

} // namespace FDE

namespace std
{
template<>
struct hash<FDE::AssetId>
{
    size_t operator()(const FDE::AssetId& id) const { return hash<string>()(id.str()); }
};
} // namespace std
