#include "FDE/pch.hpp"
#include "FDE/Runtime/RuntimeSession.hpp"
#include "FDE/Asset/AssetManager.hpp"
#include "FDE/Core/FileSystem.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/Project/ProjectDescriptor.hpp"
#include "FDE/Scene/World.hpp"
#include <glm/glm.hpp>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace FDE
{

namespace
{

float ReadScene3DNavSensitivityFromFordEditorCfg(const std::string& projectRoot)
{
    namespace fs = std::filesystem;
    fs::path cfg = fs::path(projectRoot) / "FordEditor.cfg";
    std::ifstream in(cfg);
    if (!in)
        return 1.0f;

    auto trim = [](std::string& s) {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
            s.erase(s.begin());
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
            s.pop_back();
    };

    std::string line;
    bool inScene = false;
    while (std::getline(in, line))
    {
        trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        if (line.front() == '[' && line.back() == ']')
        {
            inScene = (line == "[Scene]");
            continue;
        }
        if (!inScene)
            continue;
        constexpr const char* kKey = "Scene3DNavSensitivity=";
        if (line.compare(0, std::strlen(kKey), kKey) != 0)
            continue;
        try
        {
            return std::stof(line.substr(std::strlen(kKey)));
        }
        catch (const std::exception&)
        {
            return 1.0f;
        }
    }
    return 1.0f;
}

} // namespace

RuntimeSession::RuntimeSession() = default;

RuntimeSession::~RuntimeSession()
{
    Shutdown();
}

bool RuntimeSession::LoadFromFProjectFile(const std::string& absoluteFprojectPath, std::string& outError)
{
    Shutdown();

    namespace fs = std::filesystem;
    fs::path fpath(absoluteFprojectPath);
    if (!fs::exists(fpath) || !fs::is_regular_file(fpath))
    {
        outError = "Invalid .fproject path: " + absoluteFprojectPath;
        return false;
    }

    std::string projectRoot = fpath.parent_path().string();
    m_world = std::make_unique<World>();
    if (!ProjectDescriptor::LoadFromFile(absoluteFprojectPath, m_descriptor, outError, m_world.get()))
    {
        m_world.reset();
        return false;
    }

    FileSystem::SetProjectRoot(projectRoot);
    m_assets = std::make_unique<AssetManager>();
    m_assets->Initialize(projectRoot);

    m_scene3DNavSensitivity = ReadScene3DNavSensitivityFromFordEditorCfg(projectRoot);

    m_camera.SetPosition(glm::vec3(0.0f, 4.0f, 12.0f));
    return true;
}

void RuntimeSession::Shutdown()
{
    if (m_assets)
    {
        m_assets->Shutdown();
        m_assets.reset();
    }
    m_world.reset();
}

} // namespace FDE
