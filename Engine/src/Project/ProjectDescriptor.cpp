#include "FDE/pch.hpp"
#include "FDE/Project/ProjectDescriptor.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Scene/Object.hpp"
#include "FDE/Scene/Scene.hpp"
#include "FDE/Scene/Scene2D.hpp"
#include "FDE/Scene/World.hpp"
#include <glm/glm.hpp>
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
constexpr const char* KEY_WORLD = "world";
constexpr const char* KEY_ACTIVE_SCENE = "activeScene";
constexpr const char* KEY_SCENES = "scenes";
constexpr const char* KEY_SCENE_NAME = "name";
constexpr const char* KEY_SCENE_TYPE = "type";
constexpr const char* KEY_SCENE_TYPE_2D = "Scene2D";
constexpr const char* KEY_SCENE_TYPE_BASE = "Scene";
constexpr const char* KEY_OBJECTS = "objects";
constexpr const char* KEY_TAG = "tag";
constexpr const char* KEY_TAG_NAME = "name";
constexpr const char* KEY_TRANSFORM2D = "transform2D";
constexpr const char* KEY_POSITION = "position";
constexpr const char* KEY_ROTATION = "rotation";
constexpr const char* KEY_SCALE = "scale";
constexpr const char* KEY_MESH = "mesh";
constexpr const char* MESH_BUILTIN_TRIANGLE = "builtin:triangle";
} // namespace

static json11::Json SerializeWorld(const World& world);
static bool DeserializeWorld(const json11::Json& json, World& world);

bool ProjectDescriptor::LoadFromDirectory(const std::string& projectRoot, ProjectDescriptor& outDescriptor,
                                          std::string& outError, World* outWorld)
{
    namespace fs = std::filesystem;
    fs::path root(projectRoot);
    if (!fs::exists(root) || !fs::is_directory(root))
    {
        outError = "Project root is not a valid directory: " + projectRoot;
        return false;
    }
    fs::path fprojectPath = root / GetFileName();
    return LoadFromFile(fprojectPath.string(), outDescriptor, outError, outWorld);
}

bool ProjectDescriptor::LoadFromFile(const std::string& fprojectPath, ProjectDescriptor& outDescriptor,
                                     std::string& outError, World* outWorld)
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

    if (outWorld)
    {
        auto itWorld = obj.find(KEY_WORLD);
        if (itWorld != obj.end() && itWorld->second.is_object())
        {
            if (!DeserializeWorld(itWorld->second, *outWorld))
            {
                outError = "Failed to deserialize world data";
                return false;
            }
        }
    }

    return true;
}

bool ProjectDescriptor::SaveToDirectory(const std::string& projectRoot, std::string& outError,
                                         const World* world) const
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

    if (world)
    {
        obj[KEY_WORLD] = SerializeWorld(*world);
    }

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

static json11::Json SerializeWorld(const World& world)
{
    json11::Json::object worldObj;

    if (const Scene* active = world.GetActiveScene())
        worldObj[KEY_ACTIVE_SCENE] = active->GetName();
    else
        worldObj[KEY_ACTIVE_SCENE] = "";

    json11::Json::array scenesArr;
    for (const std::string& sceneName : world.GetSceneNames())
    {
        const Scene* scene = world.GetScene(sceneName);
        if (!scene)
            continue;

        json11::Json::object sceneObj;
        sceneObj[KEY_SCENE_NAME] = sceneName;
        sceneObj[KEY_SCENE_TYPE] =
            (dynamic_cast<const Scene2D*>(scene) != nullptr) ? KEY_SCENE_TYPE_2D : KEY_SCENE_TYPE_BASE;

        json11::Json::array objectsArr;
        const entt::registry& reg = scene->GetRegistry();
        for (auto entity : reg.view<entt::entity>())
        {
            json11::Json::object objData;

            if (const TagComponent* tag = reg.try_get<TagComponent>(entity))
            {
                json11::Json::object tagObj;
                tagObj[KEY_TAG_NAME] = tag->name;
                objData[KEY_TAG] = std::move(tagObj);
            }

            if (const Transform2DComponent* transform = reg.try_get<Transform2DComponent>(entity))
            {
                json11::Json::object transformObj;
                transformObj[KEY_POSITION] =
                    json11::Json::array{transform->position.x, transform->position.y};
                transformObj[KEY_ROTATION] = transform->rotation;
                transformObj[KEY_SCALE] =
                    json11::Json::array{transform->scale.x, transform->scale.y};
                objData[KEY_TRANSFORM2D] = std::move(transformObj);
            }

            if (const Mesh2DComponent* mesh = reg.try_get<Mesh2DComponent>(entity))
            {
                std::string meshVal = mesh->meshAsset;
                if (meshVal.empty() && mesh->vertexArray && mesh->vertexArray->GetIndexCount() == 3)
                    meshVal = MESH_BUILTIN_TRIANGLE;
                if (!meshVal.empty())
                    objData[KEY_MESH] = meshVal;
            }

            objectsArr.push_back(std::move(objData));
        }
        sceneObj[KEY_OBJECTS] = std::move(objectsArr);
        scenesArr.push_back(std::move(sceneObj));
    }
    worldObj[KEY_SCENES] = std::move(scenesArr);

    return json11::Json(std::move(worldObj));
}

static bool DeserializeWorld(const json11::Json& json, World& world)
{
    if (!json.is_object())
        return false;

    const auto& obj = json.object_items();
    std::string activeSceneName;
    auto itActive = obj.find(KEY_ACTIVE_SCENE);
    if (itActive != obj.end() && itActive->second.is_string())
        activeSceneName = itActive->second.string_value();

    auto itScenes = obj.find(KEY_SCENES);
    if (itScenes == obj.end() || !itScenes->second.is_array())
        return true; // No scenes is valid

    for (const json11::Json& sceneJson : itScenes->second.array_items())
    {
        if (!sceneJson.is_object())
            continue;

        const auto& sceneObj = sceneJson.object_items();
        auto itName = sceneObj.find(KEY_SCENE_NAME);
        auto itType = sceneObj.find(KEY_SCENE_TYPE);
        if (itName == sceneObj.end() || !itName->second.is_string())
            continue;

        std::string sceneName = itName->second.string_value();
        bool is2D = (itType != sceneObj.end() && itType->second.is_string() &&
                     itType->second.string_value() == KEY_SCENE_TYPE_2D);

        Scene* scene = is2D ? world.CreateScene2D(sceneName) : world.CreateScene(sceneName);
        if (!scene)
            continue;

        auto itObjects = sceneObj.find(KEY_OBJECTS);
        if (itObjects == sceneObj.end() || !itObjects->second.is_array())
            continue;

        for (const json11::Json& objJson : itObjects->second.array_items())
        {
            if (!objJson.is_object())
                continue;

            Object entity = scene->CreateObject();
            const auto& objData = objJson.object_items();

            auto itTag = objData.find(KEY_TAG);
            if (itTag != objData.end() && itTag->second.is_object())
            {
                const auto& tagObj = itTag->second.object_items();
                auto itTagName = tagObj.find(KEY_TAG_NAME);
                std::string tagName = "Object";
                if (itTagName != tagObj.end() && itTagName->second.is_string())
                    tagName = itTagName->second.string_value();
                scene->AddComponent<TagComponent>(entity, tagName);
            }
            else
            {
                scene->AddComponent<TagComponent>(entity, "Object");
            }

            auto itTransform = objData.find(KEY_TRANSFORM2D);
            if (itTransform != objData.end() && itTransform->second.is_object())
            {
                Transform2DComponent transform;
                const auto& tObj = itTransform->second.object_items();

                auto itPos = tObj.find(KEY_POSITION);
                if (itPos != tObj.end() && itPos->second.is_array())
                {
                    const auto& arr = itPos->second.array_items();
                    if (arr.size() >= 2)
                    {
                        transform.position.x = static_cast<float>(arr[0].number_value());
                        transform.position.y = static_cast<float>(arr[1].number_value());
                    }
                }

                auto itRot = tObj.find(KEY_ROTATION);
                if (itRot != tObj.end() && itRot->second.is_number())
                    transform.rotation = static_cast<float>(itRot->second.number_value());

                auto itScale = tObj.find(KEY_SCALE);
                if (itScale != tObj.end() && itScale->second.is_array())
                {
                    const auto& arr = itScale->second.array_items();
                    if (arr.size() >= 2)
                    {
                        transform.scale.x = static_cast<float>(arr[0].number_value());
                        transform.scale.y = static_cast<float>(arr[1].number_value());
                    }
                }

                scene->AddComponent<Transform2DComponent>(entity, transform);
            }

            auto itMesh = objData.find(KEY_MESH);
            if (itMesh != objData.end() && itMesh->second.is_string())
            {
                std::string meshVal = itMesh->second.string_value();
                Mesh2DComponent meshComp;
                meshComp.vertexArray = nullptr;
                meshComp.meshAsset = meshVal;
                scene->AddComponent<Mesh2DComponent>(entity, meshComp);
            }
        }
    }

    if (!activeSceneName.empty())
        world.SetActiveScene(activeSceneName);

    return true;
}

} // namespace FDE
