#include "FDE/pch.hpp"
#include "FDE/Asset/AssetRegistry.hpp"
#include <json11.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace FDE
{

namespace
{
constexpr const char* KEY_SCHEMA = "schemaVersion";
constexpr const char* KEY_ASSETS = "assets";
constexpr const char* KEY_GUID = "guid";
constexpr const char* KEY_LOGICAL = "logicalPath";
constexpr const char* KEY_TYPE = "type";
constexpr const char* KEY_HASH = "contentHash";
constexpr const char* KEY_BUILT = "builtRelativePath";
constexpr const char* KEY_DEPS = "dependencies";

std::string RegistryPath(const std::string& projectRoot)
{
    namespace fs = std::filesystem;
    return (fs::path(projectRoot) / "Library" / "AssetRegistry.json").string();
}

} // namespace

bool AssetRegistry::LoadFromProject(const std::string& projectRoot, AssetRegistry& out, std::string& outError)
{
    namespace fs = std::filesystem;
    fs::path path = fs::path(projectRoot) / "Library" / "AssetRegistry.json";
    if (!fs::exists(path))
    {
        out = AssetRegistry{};
        out.schemaVersion = 1;
        return true;
    }

    std::ifstream file(path);
    if (!file)
    {
        outError = "Failed to open AssetRegistry";
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string parseError;
    json11::Json json = json11::Json::parse(content, parseError);
    if (!parseError.empty())
    {
        outError = "Invalid AssetRegistry JSON: " + parseError;
        return false;
    }

    out = AssetRegistry{};
    if (json.is_object())
    {
        const auto& obj = json.object_items();
        auto itSchema = obj.find(KEY_SCHEMA);
        if (itSchema != obj.end() && itSchema->second.is_number())
            out.schemaVersion = itSchema->second.int_value();

        auto itAssets = obj.find(KEY_ASSETS);
        if (itAssets != obj.end() && itAssets->second.is_array())
        {
            for (const json11::Json& item : itAssets->second.array_items())
            {
                if (!item.is_object())
                    continue;
                const auto& o = item.object_items();
                AssetRecord rec;
                auto itG = o.find(KEY_GUID);
                auto itL = o.find(KEY_LOGICAL);
                if (itG == o.end() || !itG->second.is_string())
                    continue;
                rec.guid = AssetId(itG->second.string_value());
                if (!rec.guid.IsValid())
                    continue;
                if (itL != o.end() && itL->second.is_string())
                    rec.logicalPath = itL->second.string_value();

                auto itT = o.find(KEY_TYPE);
                if (itT != o.end() && itT->second.is_string())
                    rec.type = AssetTypeFromString(itT->second.string_value());

                auto itH = o.find(KEY_HASH);
                if (itH != o.end() && itH->second.is_string())
                    rec.contentHash = itH->second.string_value();

                auto itB = o.find(KEY_BUILT);
                if (itB != o.end() && itB->second.is_string())
                    rec.builtRelativePath = itB->second.string_value();

                auto itD = o.find(KEY_DEPS);
                if (itD != o.end() && itD->second.is_array())
                {
                    for (const json11::Json& d : itD->second.array_items())
                    {
                        if (d.is_string())
                        {
                            AssetId id(d.string_value());
                            if (id.IsValid())
                                rec.dependencies.push_back(std::move(id));
                        }
                    }
                }
                out.assets.push_back(std::move(rec));
            }
        }
    }

    return true;
}

bool AssetRegistry::SaveToProject(const std::string& projectRoot, std::string& outError) const
{
    namespace fs = std::filesystem;
    fs::path lib = fs::path(projectRoot) / "Library";
    std::error_code ec;
    fs::create_directories(lib, ec);

    json11::Json::array arr;
    for (const AssetRecord& rec : assets)
    {
        json11::Json::object o;
        o[KEY_GUID] = rec.guid.str();
        o[KEY_LOGICAL] = rec.logicalPath;
        o[KEY_TYPE] = AssetTypeToString(rec.type);
        o[KEY_HASH] = rec.contentHash;
        o[KEY_BUILT] = rec.builtRelativePath;
        json11::Json::array deps;
        for (const AssetId& d : rec.dependencies)
            deps.push_back(d.str());
        o[KEY_DEPS] = std::move(deps);
        arr.push_back(json11::Json(std::move(o)));
    }

    json11::Json::object root;
    root[KEY_SCHEMA] = schemaVersion;
    root[KEY_ASSETS] = std::move(arr);

    std::string jsonStr = json11::Json(std::move(root)).dump();
    std::ofstream file(RegistryPath(projectRoot));
    if (!file)
    {
        outError = "Failed to write AssetRegistry";
        return false;
    }
    file << jsonStr;
    return true;
}

const AssetRecord* AssetRegistry::FindByGuid(const AssetId& id) const
{
    for (const AssetRecord& r : assets)
    {
        if (r.guid == id)
            return &r;
    }
    return nullptr;
}

const AssetRecord* AssetRegistry::FindByLogicalPath(std::string_view logicalPath) const
{
    std::string norm = NormalizeLogicalPath(logicalPath);
    for (const AssetRecord& r : assets)
    {
        if (NormalizeLogicalPath(r.logicalPath) == norm)
            return &r;
    }
    return nullptr;
}

void AssetRegistry::Upsert(AssetRecord record)
{
    for (auto& r : assets)
    {
        if (r.guid == record.guid)
        {
            r = std::move(record);
            return;
        }
    }
    assets.push_back(std::move(record));
}

} // namespace FDE
