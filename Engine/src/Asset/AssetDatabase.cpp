#include "FDE/pch.hpp"
#include "FDE/Asset/AssetDatabase.hpp"
#include "FDE/Asset/AssetPack.hpp"
#include "FDE/Core/FileSystem.hpp"
#include "FDE/Core/Log.hpp"
#include <filesystem>
#include <fstream>
#include <vector>

namespace FDE
{

namespace
{

constexpr const char* ASSETS_DIR = "Assets";
constexpr const char* LIBRARY_DIR = "Library";
constexpr const char* BUILT_DIR = "Library/Built";
constexpr const char* BUILD_DIR = "Build";

uint64_t HashBytesFnv1a64(const uint8_t* data, size_t len)
{
    constexpr uint64_t offset = 14695981039346656037ull;
    constexpr uint64_t prime = 1099511628211ull;
    uint64_t h = offset;
    for (size_t i = 0; i < len; ++i)
    {
        h ^= data[i];
        h *= prime;
    }
    return h;
}

std::string HashFileToHex(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        return {};
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    uint64_t h = HashBytesFnv1a64(buffer.data(), buffer.size());
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
    return std::string(buf);
}

AssetType TypeFromExtension(const std::filesystem::path& ext)
{
    std::string e = ext.string();
    for (char& c : e)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" || e == ".tga" || e == ".hdr")
        return AssetType::Texture2D;
    if (e == ".fdshader")
        return AssetType::Shader;
    if (e == ".fdemesh")
        return AssetType::Mesh2D;
    if (e == ".obj" || e == ".fbx" || e == ".3ds" || e == ".dae" || e == ".gltf" || e == ".glb")
        return AssetType::Mesh3D;
    return AssetType::Unknown;
}

std::string RelativeToRoot(const std::filesystem::path& root, const std::filesystem::path& absolute)
{
    std::error_code ec;
    auto rel = std::filesystem::relative(absolute, root, ec);
    if (ec)
        return absolute.string();
    std::string s = rel.generic_string();
    for (char& c : s)
    {
        if (c == '\\')
            c = '/';
    }
    return s;
}

bool CopyFileSafe(const std::filesystem::path& from, const std::filesystem::path& to)
{
    std::error_code ec;
    std::filesystem::create_directories(to.parent_path(), ec);
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

} // namespace

bool AssetDatabase::EnsureLibraryLayout(const std::string& projectRoot, std::string& outError)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(fs::path(projectRoot) / ASSETS_DIR, ec);
    fs::create_directories(fs::path(projectRoot) / LIBRARY_DIR, ec);
    fs::create_directories(fs::path(projectRoot) / BUILT_DIR, ec);
    fs::create_directories(fs::path(projectRoot) / BUILD_DIR, ec);
    if (ec)
    {
        outError = ec.message();
        return false;
    }
    return true;
}

bool AssetDatabase::ImportAssetFile(const std::string& projectRoot, const std::string& assetFileAbsolute,
                                    std::string& outError)
{
    namespace fs = std::filesystem;
    fs::path root(projectRoot);
    fs::path file(assetFileAbsolute);
    if (!fs::exists(file) || !fs::is_regular_file(file))
    {
        outError = "Not a file: " + assetFileAbsolute;
        return false;
    }

    std::string logical = RelativeToRoot(root, file);
    if (logical.size() < 7 || logical.compare(0, 7, "Assets/") != 0)
    {
        outError = "File must live under Assets/: " + logical;
        return false;
    }

    if (!EnsureLibraryLayout(projectRoot, outError))
        return false;

    AssetRegistry reg;
    if (!AssetRegistry::LoadFromProject(projectRoot, reg, outError))
        return false;

    AssetType type = TypeFromExtension(file.extension());
    if (type == AssetType::Unknown)
    {
        outError = "Unsupported asset extension: " + file.extension().string();
        return false;
    }

    const AssetRecord* existing = reg.FindByLogicalPath(logical);
    AssetId guid = existing ? existing->guid : AssetId::Generate();
    std::string hash = HashFileToHex(file);
    if (existing && existing->contentHash == hash && !existing->builtRelativePath.empty())
        return true;

    std::string ext = file.extension().string();
    std::string builtRelative = std::string("Library/Built/") + guid.str() + ext;
    fs::path dest = root / builtRelative;
    if (!CopyFileSafe(file, dest))
    {
        outError = "Failed to copy to Built";
        return false;
    }

    AssetRecord rec;
    rec.guid = guid;
    rec.logicalPath = logical;
    rec.type = type;
    rec.contentHash = hash;
    rec.builtRelativePath = builtRelative;
    reg.Upsert(std::move(rec));

    if (!reg.SaveToProject(projectRoot, outError))
        return false;
    FDE_LOG_CLIENT_INFO("Asset imported: {}", logical);
    return true;
}

bool AssetDatabase::RescanAssets(const std::string& projectRoot, std::string& outError)
{
    namespace fs = std::filesystem;
    if (!EnsureLibraryLayout(projectRoot, outError))
        return false;

    fs::path assetsDir = fs::path(projectRoot) / ASSETS_DIR;
    if (!fs::exists(assetsDir))
        return true;

    for (const auto& entry : fs::recursive_directory_iterator(assetsDir))
    {
        if (!entry.is_regular_file())
            continue;
        fs::path p = entry.path();
        if (TypeFromExtension(p.extension()) == AssetType::Unknown)
            continue;
        if (!ImportAssetFile(projectRoot, p.string(), outError))
            FDE_LOG_CLIENT_WARN("Asset import skipped: {} ({})", p.string(), outError);
    }
    return true;
}

bool AssetDatabase::BuildFdepack(const std::string& projectRoot, const std::string& outputRelativePath,
                                 std::string& outError)
{
    namespace fs = std::filesystem;
    AssetRegistry reg;
    if (!AssetRegistry::LoadFromProject(projectRoot, reg, outError))
        return false;

    std::vector<AssetPackEntry> entries;
    fs::path root(projectRoot);

    for (const AssetRecord& rec : reg.assets)
    {
        if (rec.builtRelativePath.empty())
            continue;
        fs::path blobPath = root / rec.builtRelativePath;
        if (!fs::exists(blobPath))
        {
            FDE_LOG_CLIENT_WARN("Missing built file for {}", rec.logicalPath);
            continue;
        }

        std::ifstream file(blobPath, std::ios::binary);
        if (!file)
            continue;
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        AssetPackEntry e;
        e.guid = rec.guid;
        e.type = rec.type;
        e.logicalPath = rec.logicalPath;
        e.uncompressedBytes = std::move(data);
        entries.push_back(std::move(e));
    }

    fs::path out = root / outputRelativePath;
    std::error_code ec;
    fs::create_directories(out.parent_path(), ec);
    if (!AssetPackWriter::WriteFile(out.string(), std::move(entries), outError))
        return false;

    FDE_LOG_CLIENT_INFO("Wrote fdepack: {}", out.string());
    return true;
}

} // namespace FDE
