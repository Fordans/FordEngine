#include "FDE/pch.hpp"
#include "FDE/Scene/Object.hpp"
#include "FDE/Scene/Scene.hpp"

namespace FDE
{

Object::Object(entt::entity entity, Scene* scene) : m_entity(entity), m_scene(scene) {}

bool Object::IsValid() const
{
    return m_scene != nullptr && m_scene->IsValid(*this);
}

} // namespace FDE
