#include "FDE/pch.hpp"
#include "FDE/Scene/Scene.hpp"

namespace FDE
{

Scene::Scene(const std::string& name) : m_name(name) {}

Scene::~Scene() = default;

Object Scene::CreateObject()
{
    entt::entity entity = m_registry.create();
    return Object(entity, this);
}

void Scene::DestroyObject(Object object)
{
    if (IsValid(object))
    {
        m_registry.destroy(object.GetEntity());
    }
}

bool Scene::IsValid(Object object) const
{
    return m_registry.valid(object.GetEntity());
}

} // namespace FDE
