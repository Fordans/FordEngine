#include "FDE/pch.hpp"
#include "FDE/Editor/EditorScene3DTools.hpp"
#include "FDE/Renderer/Camera3D.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Scene/Scene.hpp"
#include "imgui.h"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

namespace FDE
{

namespace
{

glm::mat4 BuildModelMatrix(const Transform3DComponent& t)
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), t.position);
    model = glm::rotate(model, t.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, t.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, t.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, t.scale);
    return model;
}

glm::mat4 BuildRotationMatrix(const Transform3DComponent& t)
{
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), t.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    R = glm::rotate(R, t.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    R = glm::rotate(R, t.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    return R;
}

void ScreenToWorldRay(glm::vec2 mouseScreen, const ImVec2& imgMin, const ImVec2& imgMax, const glm::mat4& view,
                      const glm::mat4& proj, glm::vec3& outOrigin, glm::vec3& outDir)
{
    float w = imgMax.x - imgMin.x;
    float h = imgMax.y - imgMin.y;
    if (w < 1.f || h < 1.f)
    {
        outOrigin = glm::vec3(0.f);
        outDir = glm::vec3(0.f, 0.f, -1.f);
        return;
    }
    float sx = (mouseScreen.x - imgMin.x) / w;
    float sy = (imgMax.y - mouseScreen.y) / h;
    float ndcX = sx * 2.f - 1.f;
    float ndcY = sy * 2.f - 1.f;
    glm::mat4 inv = glm::inverse(proj * view);
    glm::vec4 p0 = inv * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 p1 = inv * glm::vec4(ndcX, ndcY, 1.f, 1.f);
    p0 /= p0.w;
    p1 /= p1.w;
    outOrigin = glm::vec3(p0);
    outDir = glm::normalize(glm::vec3(p1 - p0));
}

bool RayAABBSimple(const glm::vec3& o, const glm::vec3& d, const glm::vec3& bmin, const glm::vec3& bmax,
                   float& tHit)
{
    float tmin = 0.f;
    float tmax = 1e30f;
    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(d[i]) < 1e-8f)
        {
            if (o[i] < bmin[i] || o[i] > bmax[i])
                return false;
            continue;
        }
        float invD = 1.f / d[i];
        float t0 = (bmin[i] - o[i]) * invD;
        float t1 = (bmax[i] - o[i]) * invD;
        if (invD < 0.f)
            std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax < tmin)
            return false;
    }
    tHit = tmin;
    return tHit >= 0.f && tHit < 1e29f;
}

float DistRayToSegment(glm::vec3 ro, glm::vec3 rd, glm::vec3 sa, glm::vec3 sb)
{
    glm::vec3 v = sb - sa;
    float best = 1e10f;
    const int steps = 24;
    for (int i = 0; i <= steps; ++i)
    {
        float k = static_cast<float>(i) / static_cast<float>(steps);
        glm::vec3 p = sa + v * k;
        glm::vec3 w = p - ro;
        float t = glm::dot(w, rd);
        if (t < 0.f)
            t = 0.f;
        glm::vec3 proj = ro + rd * t;
        best = std::min(best, glm::length(p - proj));
    }
    return best;
}

bool RayPlaneHit(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& planeP, const glm::vec3& planeN,
                 float& t, glm::vec3& hit)
{
    float denom = glm::dot(rd, planeN);
    if (std::abs(denom) < 1e-8f)
        return false;
    t = glm::dot(planeP - ro, planeN) / denom;
    if (t < 0.f)
        return false;
    hit = ro + rd * t;
    return true;
}

/// Ray vs infinite plane; allows t < 0 (behind ray origin) for stable gizmo dragging.
bool RayPlaneHitAnyT(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& planeP, const glm::vec3& planeN,
                     float& t, glm::vec3& hit)
{
    float denom = glm::dot(rd, planeN);
    if (std::abs(denom) < 1e-7f)
        return false;
    t = glm::dot(planeP - ro, planeN) / denom;
    hit = ro + rd * t;
    return true;
}

float GizmoLengthWorld(const Camera3D& camera, const glm::vec3& pivot)
{
    float d = glm::length(camera.GetPosition() - pivot);
    return std::clamp(d * 0.14f, 0.2f, 10.f);
}

int PickGizmoAxisTranslateOrScale(Scene3DTransformMode mode, const glm::vec3& ro, const glm::vec3& rd,
                                  const glm::vec3& pivot, const glm::mat4& R, float gizmoLen)
{
    glm::vec3 ax = glm::normalize(glm::vec3(R[0]));
    glm::vec3 ay = glm::normalize(glm::vec3(R[1]));
    glm::vec3 az = glm::normalize(glm::vec3(R[2]));
    float thr = std::clamp(0.035f * gizmoLen, 0.02f, 0.25f);
    float d1 = DistRayToSegment(ro, rd, pivot, pivot + ax * gizmoLen);
    float d2 = DistRayToSegment(ro, rd, pivot, pivot + ay * gizmoLen);
    float d3 = DistRayToSegment(ro, rd, pivot, pivot + az * gizmoLen);
    if (d1 <= d2 && d1 <= d3 && d1 < thr)
        return 1;
    if (d2 <= d3 && d2 < thr)
        return 2;
    if (d3 < thr)
        return 3;
    (void)mode;
    return 0;
}

int PickGizmoAxisRotate(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& pivot, const glm::mat4& R,
                        float radius)
{
    glm::vec3 axes[3] = {glm::normalize(glm::vec3(R[0])), glm::normalize(glm::vec3(R[1])),
                         glm::normalize(glm::vec3(R[2]))};
    float thr = std::clamp(0.06f * radius, 0.03f, 0.35f);
    int best = 0;
    float bestErr = 1e10f;
    float bestT = 1e10f;
    for (int i = 0; i < 3; ++i)
    {
        float t;
        glm::vec3 hit;
        if (!RayPlaneHit(ro, rd, pivot, axes[i], t, hit))
            continue;
        glm::vec3 q = hit - pivot;
        q -= axes[i] * glm::dot(q, axes[i]);
        float rad = glm::length(q);
        float err = std::abs(rad - radius);
        if (err < thr && (best == 0 || err < bestErr || (std::abs(err - bestErr) < 1e-4f && t < bestT)))
        {
            best = i + 1;
            bestErr = err;
            bestT = t;
        }
    }
    return best;
}

int PickGizmoAxis(Scene3DTransformMode mode, const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& pivot,
                  const Transform3DComponent& tr, float gizmoLen)
{
    glm::mat4 R = BuildRotationMatrix(tr);
    if (mode == Scene3DTransformMode::Rotate)
        return PickGizmoAxisRotate(ro, rd, pivot, R, gizmoLen * 0.95f);
    return PickGizmoAxisTranslateOrScale(mode, ro, rd, pivot, R, gizmoLen);
}

bool PlaneForAxisDrag(const Camera3D& cam, const glm::vec3& axisWorld, const glm::vec3& mouseRayDir,
                      glm::vec3& outN)
{
    const glm::vec3 axis = glm::normalize(axisWorld);
    const glm::vec3 fwd = cam.GetForward();
    // Plane contains the axis and is "most vertical" to the view ray — stable for Y-axis drags.
    outN = glm::cross(axis, fwd);
    float len = glm::length(outN);
    if (len < 0.02f)
        outN = glm::cross(axis, glm::vec3(1.f, 0.f, 0.f));
    len = glm::length(outN);
    if (len < 0.02f)
        outN = glm::cross(axis, glm::vec3(0.f, 0.f, 1.f));
    len = glm::length(outN);
    if (len < 0.02f)
        return false;
    outN /= len;
    // Prefer normal facing the picking ray so intersections are in front of the camera more often.
    if (glm::dot(mouseRayDir, outN) < 0.f)
        outN = -outN;
    return true;
}

GLuint s_lineVao = 0;
GLuint s_lineVbo = 0;

void FlushLineVerts(const std::vector<float>& interleaved, const glm::mat4& view, const glm::mat4& proj,
                    const glm::vec4& color)
{
    if (interleaved.size() < 6)
        return;
    Shader* simple = Renderer::GetSimpleShader();
    if (!simple)
        return;
    if (s_lineVao == 0)
    {
        glGenVertexArrays(1, &s_lineVao);
        glGenBuffers(1, &s_lineVbo);
    }
    glBindVertexArray(s_lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_lineVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(interleaved.size() * sizeof(float)), interleaved.data(),
                 GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);

    Renderer::SetShader(simple);
    simple->Bind();
    simple->SetVec4("u_Color", color);
    Renderer::SetMVP(glm::mat4(1.f), view, proj);
    const GLuint n = static_cast<GLuint>(interleaved.size() / 3u);
    Renderer::DrawLines(n);
    glBindVertexArray(0);
    Renderer::UseDefaultShader();
}

void AppendArrowX(std::vector<float>& v, float L, float shrink)
{
    float h = L * shrink;
    // shaft
    v.insert(v.end(), {0.f, 0.f, 0.f, L * 0.78f, 0.f, 0.f});
    // head
    v.insert(v.end(), {L, 0.f, 0.f, L * 0.78f, h, 0.f, L, 0.f, 0.f, L * 0.78f, -h, 0.f,
                       L, 0.f, 0.f, L * 0.78f, 0.f, h,  L, 0.f, 0.f, L * 0.78f, 0.f, -h});
}

void AppendCircleXY(std::vector<float>& v, float R, int segs)
{
    float prevX = R;
    float prevY = 0.f;
    for (int i = 1; i <= segs; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(segs) * glm::two_pi<float>();
        float x = std::cos(t) * R;
        float y = std::sin(t) * R;
        v.insert(v.end(), {prevX, prevY, 0.f, x, y, 0.f});
        prevX = x;
        prevY = y;
    }
}

void TransformLineVerts(const std::vector<float>& local, const glm::mat4& basis, std::vector<float>& out)
{
    out.clear();
    out.reserve(local.size());
    for (size_t i = 0; i + 2 < local.size(); i += 3)
    {
        glm::vec4 a = basis * glm::vec4(local[i], local[i + 1], local[i + 2], 1.f);
        out.push_back(a.x);
        out.push_back(a.y);
        out.push_back(a.z);
    }
}

} // namespace

entt::entity TryPickMesh3DEntity(Scene& scene, glm::vec2 mouseScreen, const ImVec2& imgMin, const ImVec2& imgMax,
                                 const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight)
{
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix(viewportWidth, viewportHeight);
    glm::vec3 ro, rd;
    ScreenToWorldRay(mouseScreen, imgMin, imgMax, view, proj, ro, rd);
    if (glm::length(rd) < 0.01f)
        return entt::null;

    float bestT = 1e30f;
    entt::entity best = entt::null;
    auto& reg = scene.GetRegistry();
    auto viewMeshes = reg.view<Mesh3DComponent, Transform3DComponent>();
    for (auto e : viewMeshes)
    {
        auto& mesh = viewMeshes.get<Mesh3DComponent>(e);
        auto& tr = viewMeshes.get<Transform3DComponent>(e);
        if (!mesh.vertexArray || mesh.vertexArray->GetIndexCount() == 0)
            continue;
        glm::mat4 model = BuildModelMatrix(tr);
        glm::mat4 inv = glm::inverse(model);
        glm::vec3 lo = glm::vec3(inv * glm::vec4(ro, 1.f));
        glm::vec3 ld = glm::normalize(glm::vec3(inv * glm::vec4(rd, 0.f)));
        glm::vec3 bmin(-0.5f);
        glm::vec3 bmax(0.5f);
        float t;
        if (RayAABBSimple(lo, ld, bmin, bmax, t) && t >= 0.f && t < bestT)
        {
            bestT = t;
            best = e;
        }
    }
    return best;
}

void DrawMesh3DSelectionOutline(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
                                const std::shared_ptr<VertexArray>& meshVAO)
{
    if (!meshVAO || meshVAO->GetIndexCount() == 0)
        return;
    Shader* simple = Renderer::GetSimpleShader();
    if (!simple)
        return;

    // Stencil shell outline: no interior triangulation lines (unlike GL_LINE polygon mode).
    // Thickness scales with outlineScale — larger delta = bolder rim.
    constexpr float kOutlineScale = 1.015f;

    GLboolean depthTestWas = glIsEnabled(GL_DEPTH_TEST);
    GLint depthFunc = GL_LEQUAL;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    GLboolean colorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    GLboolean stencilWas = glIsEnabled(GL_STENCIL_TEST);
    GLint stencilFunc = GL_ALWAYS;
    GLint stencilRef = 0;
    GLint stencilMask = 0xFF;
    GLint stencilFail = GL_KEEP;
    GLint stencilPassDepthFail = GL_KEEP;
    GLint stencilPassDepthPass = GL_KEEP;
    glGetIntegerv(GL_STENCIL_FUNC, &stencilFunc);
    glGetIntegerv(GL_STENCIL_REF, &stencilRef);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &stencilMask);
    glGetIntegerv(GL_STENCIL_FAIL, &stencilFail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &stencilPassDepthFail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &stencilPassDepthPass);
    GLint stencilWriteMask = 0xFF;
    glGetIntegerv(GL_STENCIL_WRITEMASK, &stencilWriteMask);
    GLboolean cullWas = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    if (!depthTestWas)
        glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    Renderer::SetShader(simple);
    simple->Bind();
    Renderer::SetMVP(model, view, projection);
    RenderCommand::DrawIndexed(meshVAO);

    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const glm::mat4 outlineModel = model * glm::scale(glm::mat4(1.f), glm::vec3(kOutlineScale));
    simple->SetVec4("u_Color", glm::vec4(1.0f, 0.58f, 0.12f, 1.0f));
    Renderer::SetMVP(outlineModel, view, projection);
    RenderCommand::DrawIndexed(meshVAO);

    if (depthTestWas)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    glDepthFunc(depthFunc);
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    glStencilMask(static_cast<GLuint>(stencilWriteMask));
    glStencilFunc(stencilFunc, stencilRef, stencilMask);
    glStencilOp(stencilFail, stencilPassDepthFail, stencilPassDepthPass);
    if (stencilWas)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
    if (cullWas)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    Renderer::UseDefaultShader();
}

void Scene3D_UpdateGizmoInteraction(Scene3DTransformMode mode, Scene& scene, Object& selected,
                                    const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                                    const ImVec2& imgMin, const ImVec2& imgMax, Scene3DGizmoState& state,
                                    bool viewportHovered)
{
    ImGuiIO& io = ImGui::GetIO();
    if (!selected.IsValid() || !scene.IsValid(selected) || !scene.HasComponent<Transform3DComponent>(selected))
    {
        state = Scene3DGizmoState{};
        return;
    }

    if (state.lastSelection != selected.GetEntity())
    {
        state.dragging = false;
        state.activeAxis = 0;
        state.hoveredAxis = 0;
        state.lastSelection = selected.GetEntity();
    }

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix(viewportWidth, viewportHeight);
    glm::vec2 mouse(io.MousePos.x, io.MousePos.y);
    glm::vec3 ro, rd;
    ScreenToWorldRay(mouse, imgMin, imgMax, view, proj, ro, rd);

    auto* tr = scene.GetComponent<Transform3DComponent>(selected);
    glm::vec3 pivot = tr->position;
    float gLen = GizmoLengthWorld(camera, pivot);
    glm::mat4 R = BuildRotationMatrix(*tr);

    if (state.dragging)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            state.dragging = false;
            state.activeAxis = 0;
        }
        else if (state.activeAxis >= 1 && state.activeAxis <= 3)
        {
            glm::vec3 axisWorld = glm::normalize(glm::vec3(R[state.activeAxis - 1]));
            if (mode == Scene3DTransformMode::Position)
            {
                const glm::vec3 planeN = state.dragPlaneNormal;
                if (glm::length(planeN) > 1e-6f)
                {
                    float t;
                    glm::vec3 hit;
                    if (RayPlaneHitAnyT(ro, rd, tr->position, planeN, t, hit))
                    {
                        const float step = glm::dot(hit - state.dragPlaneLastHit, axisWorld);
                        state.dragAxisCumulative += step;
                        state.dragPlaneLastHit = hit;
                        tr->position = state.startPosition + axisWorld * state.dragAxisCumulative;
                    }
                }
            }
            else if (mode == Scene3DTransformMode::Scale)
            {
                const glm::vec3 planeN = state.dragPlaneNormal;
                if (glm::length(planeN) > 1e-6f)
                {
                    float t;
                    glm::vec3 hit;
                    if (RayPlaneHitAnyT(ro, rd, tr->position, planeN, t, hit))
                    {
                        const float step = glm::dot(hit - state.dragPlaneLastHit, axisWorld);
                        state.dragAxisCumulative += step;
                        state.dragPlaneLastHit = hit;
                        tr->scale = state.startScale;
                        tr->scale[state.activeAxis - 1] =
                            std::max(0.05f, state.startScale[state.activeAxis - 1] + state.dragAxisCumulative * 2.5f);
                    }
                }
            }
            else // Rotate
            {
                float t;
                glm::vec3 hit;
                if (RayPlaneHit(ro, rd, pivot, axisWorld, t, hit))
                {
                    glm::vec3 v0 = state.dragHitStart - pivot;
                    v0 -= axisWorld * glm::dot(v0, axisWorld);
                    glm::vec3 v1 = hit - pivot;
                    v1 -= axisWorld * glm::dot(v1, axisWorld);
                    float l0 = glm::length(v0);
                    float l1 = glm::length(v1);
                    if (l0 > 1e-5f && l1 > 1e-5f)
                    {
                        v0 /= l0;
                        v1 /= l1;
                        float s = glm::dot(axisWorld, glm::cross(v0, v1));
                        float c = glm::clamp(glm::dot(v0, v1), -1.f, 1.f);
                        float ang = std::atan2(s, c);
                        tr->rotation = state.startRotation;
                        if (state.activeAxis == 1)
                            tr->rotation.x += ang;
                        else if (state.activeAxis == 2)
                            tr->rotation.y += ang;
                        else
                            tr->rotation.z += ang;
                        state.dragHitStart = hit;
                        state.startRotation = tr->rotation;
                    }
                }
            }
        }
    }
    else if (viewportHovered && glm::length(rd) > 0.01f)
    {
        state.hoveredAxis = PickGizmoAxis(mode, ro, rd, pivot, *tr, gLen);
    }
    else
        state.hoveredAxis = 0;
}

void Scene3D_OnViewportPrimaryClick(Scene3DTransformMode mode, Scene& scene, Object& selected,
                                    const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                                    const ImVec2& imgMin, const ImVec2& imgMax, glm::vec2 mouseScreen,
                                    Scene3DGizmoState& state)
{
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix(viewportWidth, viewportHeight);
    glm::vec3 ro, rd;
    ScreenToWorldRay(mouseScreen, imgMin, imgMax, view, proj, ro, rd);
    if (glm::length(rd) < 0.01f)
        return;

    if (selected.IsValid() && scene.IsValid(selected) && scene.HasComponent<Transform3DComponent>(selected))
    {
        auto* tr = scene.GetComponent<Transform3DComponent>(selected);
        glm::vec3 pivot = tr->position;
        float gLen = GizmoLengthWorld(camera, pivot);
        int axis = PickGizmoAxis(mode, ro, rd, pivot, *tr, gLen);
        if (axis >= 1)
        {
            glm::mat4 R = BuildRotationMatrix(*tr);
            glm::vec3 axisWorld = glm::normalize(glm::vec3(R[axis - 1]));
            state.dragging = true;
            state.activeAxis = axis;
            state.startPosition = tr->position;
            state.startScale = tr->scale;
            state.startRotation = tr->rotation;
            if (mode == Scene3DTransformMode::Rotate)
            {
                float t;
                glm::vec3 hit;
                if (RayPlaneHit(ro, rd, pivot, axisWorld, t, hit))
                    state.dragHitStart = hit;
                else
                    state.dragHitStart = pivot + axisWorld * 0.01f;
            }
            else
            {
                glm::vec3 planeN;
                if (PlaneForAxisDrag(camera, axisWorld, rd, planeN))
                {
                    state.dragPlaneNormal = planeN;
                    state.dragPlanePoint = pivot;
                    float t;
                    glm::vec3 hit;
                    if (RayPlaneHitAnyT(ro, rd, pivot, planeN, t, hit))
                        state.dragHitStart = hit;
                    else
                        state.dragHitStart = pivot;
                    state.dragPlaneLastHit = state.dragHitStart;
                    state.dragAxisCumulative = 0.f;
                }
                else
                    state.dragging = false;
            }
            return;
        }
    }

    entt::entity e = TryPickMesh3DEntity(scene, mouseScreen, imgMin, imgMax, camera, viewportWidth, viewportHeight);
    if (e != entt::null)
        selected = Object(e, &scene);
    else
        selected = Object{};
    state.dragging = false;
    state.activeAxis = 0;
    state.hoveredAxis = 0;
    state.lastSelection = selected.IsValid() ? selected.GetEntity() : entt::null;
}

void DrawScene3DGizmo(Scene3DTransformMode mode, Scene& scene, const Object& selected, const Camera3D& camera,
                      uint32_t viewportWidth, uint32_t viewportHeight, const glm::mat4& view,
                      const glm::mat4& projection, const Scene3DGizmoState& state)
{
    if (!selected.IsValid() || !scene.IsValid(selected) || !scene.HasComponent<Transform3DComponent>(selected))
        return;
    const auto* tr = scene.GetComponent<Transform3DComponent>(selected);
    glm::vec3 pivot = tr->position;
    float gLen = GizmoLengthWorld(camera, pivot);
    glm::mat4 T = glm::translate(glm::mat4(1.f), pivot);
    glm::mat4 R = BuildRotationMatrix(*tr);
    glm::mat4 basis = T * R * glm::scale(glm::mat4(1.f), glm::vec3(gLen));

    GLboolean depthWas = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    std::vector<float> local;
    std::vector<float> world;

    if (mode == Scene3DTransformMode::Rotate)
    {
        // AppendCircleXY builds a circle in local XY (z=0) = plane perpendicular to +Z (rotation around Z).
        // X ring: plane YZ  -> rotate local XY by +90° about Y sends +X to +Z, circle lies in YZ.
        // Y ring: plane XZ  -> rotate local XY by +90° about X sends +Y to -Z, circle lies in XZ.
        // Z ring: plane XY  -> basis only.
        const float R0 = 0.95f;
        AppendCircleXY(local, R0, 64);
        const glm::mat4 bx =
            basis * glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(0.f, 1.f, 0.f));
        TransformLineVerts(local, bx, world);
        FlushLineVerts(world, view, projection,
                       state.hoveredAxis == 1 || state.activeAxis == 1
                           ? glm::vec4(1.f, 0.35f, 0.35f, 1.f)
                           : glm::vec4(0.85f, 0.25f, 0.25f, 1.f));
        local.clear();
        AppendCircleXY(local, R0, 64);
        const glm::mat4 by =
            basis * glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(1.f, 0.f, 0.f));
        TransformLineVerts(local, by, world);
        FlushLineVerts(world, view, projection,
                       state.hoveredAxis == 2 || state.activeAxis == 2
                           ? glm::vec4(0.35f, 1.f, 0.35f, 1.f)
                           : glm::vec4(0.25f, 0.85f, 0.25f, 1.f));
        local.clear();
        AppendCircleXY(local, R0, 64);
        const glm::mat4 bz = basis;
        TransformLineVerts(local, bz, world);
        FlushLineVerts(world, view, projection,
                       state.hoveredAxis == 3 || state.activeAxis == 3
                           ? glm::vec4(0.4f, 0.55f, 1.f, 1.f)
                           : glm::vec4(0.25f, 0.4f, 0.9f, 1.f));
    }
    else
    {
        const float shrink = 0.12f;
        local.clear();
        AppendArrowX(local, 1.f, shrink);
        TransformLineVerts(local, basis, world);
        glm::vec4 cx = (mode == Scene3DTransformMode::Scale)
                           ? glm::vec4(0.95f, 0.85f, 0.35f, 1.f)
                           : glm::vec4(0.9f, 0.25f, 0.25f, 1.f);
        if (state.hoveredAxis == 1 || state.activeAxis == 1)
            cx = glm::vec4(1.f, 0.5f, 0.5f, 1.f);
        FlushLineVerts(world, view, projection, cx);

        local.clear();
        AppendArrowX(local, 1.f, shrink);
        glm::mat4 by = basis * glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(0.f, 0.f, 1.f));
        TransformLineVerts(local, by, world);
        glm::vec4 cy = (mode == Scene3DTransformMode::Scale)
                           ? glm::vec4(0.95f, 0.9f, 0.4f, 1.f)
                           : glm::vec4(0.25f, 0.9f, 0.25f, 1.f);
        if (state.hoveredAxis == 2 || state.activeAxis == 2)
            cy = glm::vec4(0.5f, 1.f, 0.5f, 1.f);
        FlushLineVerts(world, view, projection, cy);

        local.clear();
        AppendArrowX(local, 1.f, shrink);
        glm::mat4 bz = basis * glm::rotate(glm::mat4(1.f), -glm::half_pi<float>(), glm::vec3(0.f, 1.f, 0.f));
        TransformLineVerts(local, bz, world);
        glm::vec4 cz = (mode == Scene3DTransformMode::Scale)
                           ? glm::vec4(0.85f, 0.8f, 0.95f, 1.f)
                           : glm::vec4(0.3f, 0.45f, 1.f, 1.f);
        if (state.hoveredAxis == 3 || state.activeAxis == 3)
            cz = glm::vec4(0.55f, 0.65f, 1.f, 1.f);
        FlushLineVerts(world, view, projection, cz);
    }

    if (depthWas)
        glEnable(GL_DEPTH_TEST);
}

} // namespace FDE
