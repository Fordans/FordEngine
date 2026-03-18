#pragma once

#include "FDE/Export.hpp"
#include <entt.hpp>

namespace FDE
{

class Scene;  // forward declaration

/// Handle to an entity (object) within a scene.
/// Valid only while the owning Scene exists. Do not store across scene unload.
class FDE_API Object
{
  public:
    Object() = default;
    Object(entt::entity entity, Scene* scene);

    /// Check if this handle is valid (entity exists in its scene).
    bool IsValid() const;

    explicit operator bool() const { return IsValid(); }
    operator entt::entity() const { return m_entity; }

    entt::entity GetEntity() const { return m_entity; }
    Scene* GetScene() const { return m_scene; }

  private:
    entt::entity m_entity{entt::null};
    Scene* m_scene = nullptr;
};

} // namespace FDE
