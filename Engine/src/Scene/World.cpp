#include "FDE/pch.hpp"
#include "FDE/Scene/World.hpp"

namespace FDE
{

Scene* World::CreateScene(const std::string& name)
{
    if (name.empty() || m_scenes.find(name) != m_scenes.end())
    {
        return nullptr;
    }
    auto scene = std::make_unique<Scene>(name);
    Scene* ptr = scene.get();
    m_scenes[name] = std::move(scene);
    return ptr;
}

Scene2D* World::CreateScene2D(const std::string& name)
{
    if (name.empty() || m_scenes.find(name) != m_scenes.end())
    {
        return nullptr;
    }
    auto scene = std::make_unique<Scene2D>(name);
    Scene2D* ptr = scene.get();
    m_scenes[name] = std::move(scene);
    return ptr;
}

Scene3D* World::CreateScene3D(const std::string& name)
{
    if (name.empty() || m_scenes.find(name) != m_scenes.end())
    {
        return nullptr;
    }
    auto scene = std::make_unique<Scene3D>(name);
    Scene3D* ptr = scene.get();
    m_scenes[name] = std::move(scene);
    return ptr;
}

Scene* World::GetScene(const std::string& name)
{
    auto it = m_scenes.find(name);
    return it != m_scenes.end() ? it->second.get() : nullptr;
}

const Scene* World::GetScene(const std::string& name) const
{
    auto it = m_scenes.find(name);
    return it != m_scenes.end() ? it->second.get() : nullptr;
}

Scene2D* World::GetScene2D(const std::string& name)
{
    return dynamic_cast<Scene2D*>(GetScene(name));
}

const Scene2D* World::GetScene2D(const std::string& name) const
{
    return dynamic_cast<const Scene2D*>(GetScene(name));
}

Scene3D* World::GetScene3D(const std::string& name)
{
    return dynamic_cast<Scene3D*>(GetScene(name));
}

const Scene3D* World::GetScene3D(const std::string& name) const
{
    return dynamic_cast<const Scene3D*>(GetScene(name));
}

void World::DestroyScene(const std::string& name)
{
    auto it = m_scenes.find(name);
    if (it != m_scenes.end())
    {
        if (m_activeScene == it->second.get())
        {
            m_activeScene = nullptr;
        }
        m_scenes.erase(it);
    }
}

std::vector<std::string> World::GetSceneNames() const
{
    std::vector<std::string> names;
    names.reserve(m_scenes.size());
    for (const auto& [n, _] : m_scenes)
    {
        names.push_back(n);
    }
    return names;
}

void World::SetActiveScene(const std::string& name)
{
    if (name.empty())
    {
        m_activeScene = nullptr;
        return;
    }
    m_activeScene = GetScene(name);
}

void World::SetActiveScene(Scene* scene)
{
    if (scene == nullptr)
    {
        m_activeScene = nullptr;
        return;
    }
    for (const auto& [n, s] : m_scenes)
    {
        if (s.get() == scene)
        {
            m_activeScene = scene;
            return;
        }
    }
    m_activeScene = nullptr;
}

} // namespace FDE
