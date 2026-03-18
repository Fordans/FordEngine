#include "FDE/Project/ProjectDescriptor.hpp"
#include <json11.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace FDE
{

namespace
{
constexpr int CURRENT_SCHEMA_VERSION = 1;
constexpr const char* KEY_NAME = "name";
constexpr const char* KEY_VERSION = "version";
constexpr const char* KEY_SCHEMA_VERSION = "schemaVersion";
} // namespace

bool ProjectDescriptor::LoadFromDirectory(const std::string& projectRoot, ProjectDescriptor& outDescriptor,
                                          std::string& outError)
{
    namespace fs = std::filesystem;
    fs::path root(projectRoot);
    if (!fs::exists(root) || !fs::is_directory(root))
    {
        outError = "Project root is not a valid directory: " + projectRoot;
        return false;
    }
    fs::path fprojectPath = root / GetFileName();
    return LoadFromFile(fprojectPath.string(), outDescriptor, outError);
}

bool ProjectDescriptor::LoadFromFile(const std::string& fprojectPath, ProjectDescriptor& outDescriptor,
                                     std::string& outError)
{
    namespace fs = std::filesystem;
    fs::path path(fprojectPath);
    if (!fs::exists(path) || !fs::is_regular_file(path))
    {
        outError = ".fproject file not found: " + fprojectPath;
        return false;
    }

    std::ifstream file(path);
    if (!file)
    {
        outError = "Failed to open .fproject: " + fprojectPath;
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    std::string parseError;
    json11::Json json = json11::Json::parse(content, parseError);
    if (!parseError.empty())
    {
        outError = "Invalid JSON in .fproject: " + parseError;
        return false;
    }

    if (!json.is_object())
    {
        outError = ".fproject must be a JSON object";
        return false;
    }

    const auto& obj = json.object_items();
    auto itName = obj.find(KEY_NAME);
    auto itVersion = obj.find(KEY_VERSION);

    if (itName == obj.end() || !itName->second.is_string())
    {
        outError = ".fproject must contain a string field 'name'";
        return false;
    }
    if (itVersion == obj.end() || !itVersion->second.is_string())
    {
        outError = ".fproject must contain a string field 'version'";
        return false;
    }

    outDescriptor.name = itName->second.string_value();
    outDescriptor.version = itVersion->second.string_value();

    auto itSchema = obj.find(KEY_SCHEMA_VERSION);
    if (itSchema != obj.end() && itSchema->second.is_number())
        outDescriptor.schemaVersion = itSchema->second.int_value();
    else
        outDescriptor.schemaVersion = CURRENT_SCHEMA_VERSION;

    if (outDescriptor.schemaVersion > CURRENT_SCHEMA_VERSION)
    {
        outError = ".fproject schemaVersion " + std::to_string(outDescriptor.schemaVersion) +
                   " is newer than supported (" + std::to_string(CURRENT_SCHEMA_VERSION) + ")";
        return false;
    }

    return true;
}

bool ProjectDescriptor::SaveToDirectory(const std::string& projectRoot, std::string& outError) const
{
    namespace fs = std::filesystem;
    fs::path root(projectRoot);
    if (!fs::exists(root))
    {
        if (!fs::create_directories(root))
        {
            outError = "Failed to create project directory: " + projectRoot;
            return false;
        }
    }
    else if (!fs::is_directory(root))
    {
        outError = "Project root is not a directory: " + projectRoot;
        return false;
    }

    json11::Json::object obj;
    obj[KEY_NAME] = name;
    obj[KEY_VERSION] = version;
    obj[KEY_SCHEMA_VERSION] = schemaVersion;

    json11::Json json(std::move(obj));
    std::string content = json.dump();

    fs::path fprojectPath = root / GetFileName();
    std::ofstream file(fprojectPath);
    if (!file)
    {
        outError = "Failed to write .fproject: " + fprojectPath.string();
        return false;
    }
    file << content;
    file.close();
    return true;
}

bool ProjectDescriptor::IsProjectDirectory(const std::string& path)
{
    std::string err;
    ProjectDescriptor unused;
    return LoadFromDirectory(path, unused, err);
}

} // namespace FDE
