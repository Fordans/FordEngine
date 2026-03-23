#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Scene/Object.hpp"
#include <entt.hpp>
#include <glm/glm.hpp>
#include <memory>

struct ImVec2;

namespace FDE
{

class Scene;
class Camera3D;

enum class Scene3DTransformMode : int
{
    Position = 0,
    Rotate = 1,
    Scale = 2
};

/// Mutable interaction state for the 3D viewport gizmo (stored on EditorApplication).
struct FDE_API Scene3DGizmoState
{
    entt::entity lastSelection = entt::null;
    int hoveredAxis = 0; // 0 none, 1=x, 2=y, 3=z
    int activeAxis = 0;
    bool dragging = false;
    glm::vec3 dragPlaneNormal{0.f};
    glm::vec3 dragPlanePoint{0.f};
    glm::vec3 dragHitStart{0.f};
    /// Last plane hit for incremental translate/scale (stable vs. t<0 drops).
    glm::vec3 dragPlaneLastHit{0.f};
    float dragAxisCumulative = 0.f;
    glm::vec3 startPosition{0.f};
    glm::vec3 startScale{1.f};
    glm::vec3 startRotation{0.f};
    glm::vec3 rotatePlaneNormal{0.f};
    glm::vec2 rotateStartDir{0.f}; // in plane basis
};

/// Screen-space ray vs Mesh3D + Transform3D entities (AABB pick using \p Mesh3DComponent local bounds).
FDE_API entt::entity TryPickMesh3DEntity(Scene& scene, glm::vec2 mouseScreen, const ImVec2& imgMin,
                                         const ImVec2& imgMax, const Camera3D& camera, uint32_t viewportWidth,
                                         uint32_t viewportHeight);

FDE_API void DrawMesh3DSelectionOutline(const glm::mat4& model, const glm::mat4& view,
                                        const glm::mat4& projection,
                                        const std::shared_ptr<VertexArray>& meshVAO);

/// Per-frame: hover + drag response. Call with last-frame image rect + current mouse.
FDE_API void Scene3D_UpdateGizmoInteraction(Scene3DTransformMode mode, Scene& scene, Object& selected,
                                            const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                                            const ImVec2& imgMin, const ImVec2& imgMax, Scene3DGizmoState& state,
                                            bool viewportHovered);

/// After left-click on viewport: gizmo hit first, else mesh pick. Updates \p selected.
FDE_API void Scene3D_OnViewportPrimaryClick(Scene3DTransformMode mode, Scene& scene, Object& selected,
                                            const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                                            const ImVec2& imgMin, const ImVec2& imgMax, glm::vec2 mouseScreen,
                                            Scene3DGizmoState& state);

FDE_API void DrawScene3DGizmo(Scene3DTransformMode mode, Scene& scene, const Object& selected,
                              const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                              const glm::mat4& view, const glm::mat4& projection, const Scene3DGizmoState& state);

} // namespace FDE
