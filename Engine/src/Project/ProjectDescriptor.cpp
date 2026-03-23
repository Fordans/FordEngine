#include "FDE/pch.hpp"
#include "FDE/Project/ProjectDescriptor.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Scene/Object.hpp"
#include "FDE/Scene/Scene.hpp"
#include "FDE/Scene/Scene2D.hpp"
#include "FDE/Scene/Scene3D.hpp"
#include "FDE/Scene/World.hpp"
#include <glm/glm.hpp>
#include <json11.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

namespace FDE
{

namespace
{
constexpr int CURRENT_SCHEMA_VERSION = 2;
constexpr const char* KEY_NAME = "name";
constexpr const char* KEY_VERSION = "version";
constexpr const char* KEY_SCHEMA_VERSION = "schemaVersion";
constexpr const char* KEY_WORLD = "world";
constexpr const char* KEY_ACTIVE_SCENE = "activeScene";
constexpr const char* KEY_SCENES = "scenes";
constexpr const char* KEY_SCENE_NAME = "name";
constexpr const char* KEY_SCENE_TYPE = "type";
constexpr const char* KEY_SCENE_TYPE_2D = "Scene2D";
constexpr const char* KEY_SCENE_TYPE_3D = "Scene3D";
constexpr const char* KEY_SCENE_TYPE_BASE = "Scene";
constexpr const char* KEY_TRANSFORM3D = "transform3D";
constexpr const char* MESH_BUILTIN_CUBE = "builtin:cube";
constexpr const char* KEY_OBJECTS = "objects";
constexpr const char* KEY_TAG = "tag";
constexpr const char* KEY_TAG_NAME = "name";
constexpr const char* KEY_TRANSFORM2D = "transform2D";
constexpr const char* KEY_POSITION = "position";
constexpr const char* KEY_ROTATION = "rotation";
constexpr const char* KEY_SCALE = "scale";
constexpr const char* KEY_MESH = "mesh";
constexpr const char* MESH_BUILTIN_TRIANGLE = "builtin:triangle";
constexpr const char* KEY_SCENE_VIEW_CAMERA3D = "sceneViewCamera3D";
constexpr const char* KEY_CAM_POS = "position";
constexpr const char* KEY_CAM_YAW = "yaw";
constexpr const char* KEY_CAM_PITCH = "pitch";
constexpr const char* KEY_ALBEDO_TEXTURE = "albedoTexture";
constexpr const char* KEY_DIRECTIONAL_LIGHT = "directionalLight";
constexpr const char* KEY_DIR_LIGHT_DIRECTION = "direction";
constexpr const char* KEY_DIR_LIGHT_COLOR = "color";
constexpr const char* KEY_DIR_LIGHT_INTENSITY = "intensity";
constexpr const char* KEY_SKYBOX_CROSS = "skyboxCrossTexture";

bool MeshAssetImpliesScene3D(std::string_view mesh)
{
    if (mesh == MESH_BUILTIN_CUBE)
        return true;
    static constexpr std::string_view kEngine = "engine:";
    return mesh.size() >= kEngine.size() && mesh.compare(0, kEngine.size(), kEngine) == 0;
}
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

    auto itCam = obj.find(KEY_SCENE_VIEW_CAMERA3D);
    if (itCam != obj.end() && itCam->second.is_object())
    {
        const auto& camObj = itCam->second.object_items();
        auto itP = camObj.find(KEY_CAM_POS);
        if (itP != camObj.end() && itP->second.is_array())
        {
            const auto& arr = itP->second.array_items();
            if (arr.size() >= 3 && arr[0].is_number() && arr[1].is_number() && arr[2].is_number())
            {
                SceneViewCamera3DSnapshot snap;
                snap.positionX = static_cast<float>(arr[0].number_value());
                snap.positionY = static_cast<float>(arr[1].number_value());
                snap.positionZ = static_cast<float>(arr[2].number_value());
                auto itYaw = camObj.find(KEY_CAM_YAW);
                if (itYaw != camObj.end() && itYaw->second.is_number())
                    snap.yaw = static_cast<float>(itYaw->second.number_value());
                auto itPitch = camObj.find(KEY_CAM_PITCH);
                if (itPitch != camObj.end() && itPitch->second.is_number())
                    snap.pitch = static_cast<float>(itPitch->second.number_value());
                snap.hasValue = true;
                outDescriptor.sceneViewCamera3D = snap;
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
    obj[KEY_SCHEMA_VERSION] = CURRENT_SCHEMA_VERSION;

    if (world)
    {
        obj[KEY_WORLD] = SerializeWorld(*world);
    }

    if (sceneViewCamera3D.hasValue)
    {
        json11::Json::object camObj;
        camObj[KEY_CAM_POS] = json11::Json::array{sceneViewCamera3D.positionX, sceneViewCamera3D.positionY,
                                                  sceneViewCamera3D.positionZ};
        camObj[KEY_CAM_YAW] = sceneViewCamera3D.yaw;
        camObj[KEY_CAM_PITCH] = sceneViewCamera3D.pitch;
        obj[KEY_SCENE_VIEW_CAMERA3D] = json11::Json(std::move(camObj));
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
        const char* sceneType = KEY_SCENE_TYPE_BASE;
        if (dynamic_cast<const Scene2D*>(scene) != nullptr)
            sceneType = KEY_SCENE_TYPE_2D;
        else if (dynamic_cast<const Scene3D*>(scene) != nullptr)
            sceneType = KEY_SCENE_TYPE_3D;
        sceneObj[KEY_SCENE_TYPE] = sceneType;

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

            if (const Transform3DComponent* t3 = reg.try_get<Transform3DComponent>(entity))
            {
                json11::Json::object transformObj;
                transformObj[KEY_POSITION] =
                    json11::Json::array{t3->position.x, t3->position.y, t3->position.z};
                transformObj[KEY_ROTATION] =
                    json11::Json::array{t3->rotation.x, t3->rotation.y, t3->rotation.z};
                transformObj[KEY_SCALE] =
                    json11::Json::array{t3->scale.x, t3->scale.y, t3->scale.z};
                objData[KEY_TRANSFORM3D] = std::move(transformObj);
            }

            if (const Mesh3DComponent* mesh3 = reg.try_get<Mesh3DComponent>(entity))
            {
                std::string meshVal = mesh3->meshAsset;
                if (meshVal.empty() && mesh3->vertexArray && mesh3->vertexArray->GetIndexCount() == 36)
                    meshVal = MESH_BUILTIN_CUBE;
                if (!meshVal.empty())
                    objData[KEY_MESH] = meshVal;
                if (!mesh3->albedoTextureAsset.empty())
                    objData[KEY_ALBEDO_TEXTURE] = mesh3->albedoTextureAsset;
            }

            if (const DirectionalLightComponent* dl = reg.try_get<DirectionalLightComponent>(entity))
            {
                json11::Json::object dlobj;
                dlobj[KEY_DIR_LIGHT_DIRECTION] =
                    json11::Json::array{dl->direction.x, dl->direction.y, dl->direction.z};
                dlobj[KEY_DIR_LIGHT_COLOR] = json11::Json::array{dl->color.x, dl->color.y, dl->color.z};
                dlobj[KEY_DIR_LIGHT_INTENSITY] = dl->intensity;
                objData[KEY_DIRECTIONAL_LIGHT] = std::move(dlobj);
            }

            if (const SkyboxComponent* sky = reg.try_get<SkyboxComponent>(entity))
            {
                if (!sky->crossTextureAsset.empty())
                    objData[KEY_SKYBOX_CROSS] = sky->crossTextureAsset;
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
        std::string typeStr =
            (itType != sceneObj.end() && itType->second.is_string()) ? itType->second.string_value() : "";

        bool needScene3D = false;
        auto itObjectsPeek = sceneObj.find(KEY_OBJECTS);
        if (itObjectsPeek != sceneObj.end() && itObjectsPeek->second.is_array())
        {
            for (const json11::Json& peekObj : itObjectsPeek->second.array_items())
            {
                if (!peekObj.is_object())
                    continue;
                const auto& peekData = peekObj.object_items();
                if (peekData.find(KEY_TRANSFORM3D) != peekData.end())
                    needScene3D = true;
                auto itPeekMesh = peekData.find(KEY_MESH);
                if (itPeekMesh != peekData.end() && itPeekMesh->second.is_string()
                    && MeshAssetImpliesScene3D(itPeekMesh->second.string_value()))
                    needScene3D = true;
                if (peekData.find(KEY_DIRECTIONAL_LIGHT) != peekData.end())
                    needScene3D = true;
                if (peekData.find(KEY_SKYBOX_CROSS) != peekData.end())
                    needScene3D = true;
            }
        }

        Scene* scene = nullptr;
        if (typeStr == KEY_SCENE_TYPE_BASE && !needScene3D)
            scene = world.CreateScene(sceneName);
        else if (typeStr == KEY_SCENE_TYPE_2D && !needScene3D)
            scene = world.CreateScene2D(sceneName);
        else if (typeStr == KEY_SCENE_TYPE_3D || needScene3D)
            scene = world.CreateScene3D(sceneName);
        else
            scene = world.CreateScene3D(sceneName);
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

            auto itTransform3D = objData.find(KEY_TRANSFORM3D);
            if (itTransform3D != objData.end() && itTransform3D->second.is_object())
            {
                Transform3DComponent transform;
                const auto& tObj = itTransform3D->second.object_items();

                auto itPos = tObj.find(KEY_POSITION);
                if (itPos != tObj.end() && itPos->second.is_array())
                {
                    const auto& arr = itPos->second.array_items();
                    if (arr.size() >= 3)
                    {
                        transform.position.x = static_cast<float>(arr[0].number_value());
                        transform.position.y = static_cast<float>(arr[1].number_value());
                        transform.position.z = static_cast<float>(arr[2].number_value());
                    }
                }

                auto itRot = tObj.find(KEY_ROTATION);
                if (itRot != tObj.end() && itRot->second.is_array())
                {
                    const auto& arr = itRot->second.array_items();
                    if (arr.size() >= 3)
                    {
                        transform.rotation.x = static_cast<float>(arr[0].number_value());
                        transform.rotation.y = static_cast<float>(arr[1].number_value());
                        transform.rotation.z = static_cast<float>(arr[2].number_value());
                    }
                }

                auto itScale = tObj.find(KEY_SCALE);
                if (itScale != tObj.end() && itScale->second.is_array())
                {
                    const auto& arr = itScale->second.array_items();
                    if (arr.size() >= 3)
                    {
                        transform.scale.x = static_cast<float>(arr[0].number_value());
                        transform.scale.y = static_cast<float>(arr[1].number_value());
                        transform.scale.z = static_cast<float>(arr[2].number_value());
                    }
                }

                scene->AddComponent<Transform3DComponent>(entity, transform);
            }

            auto itDirLight = objData.find(KEY_DIRECTIONAL_LIGHT);
            if (itDirLight != objData.end() && itDirLight->second.is_object())
            {
                DirectionalLightComponent dl;
                const auto& d = itDirLight->second.object_items();
                auto itD = d.find(KEY_DIR_LIGHT_DIRECTION);
                if (itD != d.end() && itD->second.is_array())
                {
                    const auto& arr = itD->second.array_items();
                    if (arr.size() >= 3)
                    {
                        dl.direction.x = static_cast<float>(arr[0].number_value());
                        dl.direction.y = static_cast<float>(arr[1].number_value());
                        dl.direction.z = static_cast<float>(arr[2].number_value());
                    }
                }
                auto itC = d.find(KEY_DIR_LIGHT_COLOR);
                if (itC != d.end() && itC->second.is_array())
                {
                    const auto& arr = itC->second.array_items();
                    if (arr.size() >= 3)
                    {
                        dl.color.x = static_cast<float>(arr[0].number_value());
                        dl.color.y = static_cast<float>(arr[1].number_value());
                        dl.color.z = static_cast<float>(arr[2].number_value());
                    }
                }
                auto itI = d.find(KEY_DIR_LIGHT_INTENSITY);
                if (itI != d.end() && itI->second.is_number())
                    dl.intensity = static_cast<float>(itI->second.number_value());
                scene->AddComponent<DirectionalLightComponent>(entity, dl);
            }

            auto itSky = objData.find(KEY_SKYBOX_CROSS);
            if (itSky != objData.end() && itSky->second.is_string())
            {
                SkyboxComponent sb;
                sb.crossTextureAsset = itSky->second.string_value();
                scene->AddComponent<SkyboxComponent>(entity, sb);
            }

            auto itMesh = objData.find(KEY_MESH);
            if (itMesh != objData.end() && itMesh->second.is_string())
            {
                std::string meshVal = itMesh->second.string_value();
                const bool useMesh3D =
                    (dynamic_cast<Scene3D*>(scene) != nullptr) || MeshAssetImpliesScene3D(meshVal);
                if (useMesh3D)
                {
                    Mesh3DComponent meshComp;
                    meshComp.vertexArray = nullptr;
                    meshComp.meshAsset = meshVal;
                    auto itAlb = objData.find(KEY_ALBEDO_TEXTURE);
                    if (itAlb != objData.end() && itAlb->second.is_string())
                        meshComp.albedoTextureAsset = itAlb->second.string_value();
                    scene->AddComponent<Mesh3DComponent>(entity, meshComp);
                }
                else
                {
                    Mesh2DComponent meshComp;
                    meshComp.vertexArray = nullptr;
                    meshComp.meshAsset = meshVal;
                    scene->AddComponent<Mesh2DComponent>(entity, meshComp);
                }
            }

            if (dynamic_cast<Scene3D*>(scene) && scene->HasComponent<Mesh3DComponent>(entity)
                && !scene->HasComponent<Transform3DComponent>(entity))
            {
                scene->AddComponent<Transform3DComponent>(entity, Transform3DComponent{});
            }
        }
    }

    if (!activeSceneName.empty())
        world.SetActiveScene(activeSceneName);

    return true;
}

} // namespace FDE
