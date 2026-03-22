#include "FDE/pch.hpp"
#include "FDE/Asset/AssetId.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <random>

namespace FDE
{

namespace
{
bool IsHex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool IsUuidShape(std::string_view s)
{
    if (s.size() != 36)
        return false;
    for (size_t i = 0; i < 36; ++i)
    {
        char c = s[i];
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (c != '-')
                return false;
        }
        else if (!IsHex(c))
            return false;
    }
    return true;
}

void ToLowerInPlace(std::string& s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

} // namespace

AssetId::AssetId(std::string id)
{
    ToLowerInPlace(id);
    m_id = std::move(id);
}

AssetId AssetId::Generate()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 255);
    std::array<unsigned char, 16> bytes{};
    for (auto& b : bytes)
        b = static_cast<unsigned char>(dis(gen));
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0Fu) | 0x40u);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3Fu) | 0x80u);

    char buf[40];
    std::snprintf(buf, sizeof(buf),
                  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x%02x%02x", bytes[0],
                  bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9],
                  bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return AssetId(std::string(buf));
}

std::optional<AssetId> AssetId::Parse(std::string_view ref)
{
    if (ref.empty())
        return std::nullopt;

    if (ref.size() >= 4 && ref.compare(0, 4, "guid") == 0)
    {
        size_t i = 4;
        while (i < ref.size() && (ref[i] == ':' || ref[i] == '/'))
            ++i;
        std::string s(ref.substr(i));
        ToLowerInPlace(s);
        if (IsUuidShape(s))
            return AssetId(std::move(s));
        return std::nullopt;
    }

    if (ref.size() > 8 && ref.compare(0, 8, "fde://") == 0)
    {
        std::string_view rest = ref.substr(8);
        if (rest.size() >= 5 && rest.compare(0, 5, "guid/") == 0)
        {
            std::string s(std::string(rest.substr(5)));
            ToLowerInPlace(s);
            if (IsUuidShape(s))
                return AssetId(std::move(s));
        }
        return std::nullopt;
    }

    std::string s(ref);
    ToLowerInPlace(s);
    if (IsUuidShape(s))
        return AssetId(std::move(s));

    return std::nullopt;
}

bool AssetId::IsValid() const
{
    return IsUuidShape(m_id);
}

bool AssetId::LooksLikeGuidReference(std::string_view meshAsset)
{
    if (meshAsset.compare(0, 7, "builtin") == 0)
        return false;
    return static_cast<bool>(Parse(meshAsset));
}

std::string NormalizeLogicalPath(std::string_view path)
{
    std::string out;
    out.reserve(path.size());
    for (char c : path)
    {
        if (c == '\\')
            out.push_back('/');
        else
            out.push_back(c);
    }
    while (!out.empty() && out.front() == '/')
        out.erase(out.begin());
    return out;
}

} // namespace FDE
