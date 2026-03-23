#include "FDE/pch.hpp"
#include "FDE/Scene/Scene3D.hpp"
#include "FDE/Asset/AssetManager.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/Shader.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace FDE
{

namespace
{

void ApplyMesh3DLightingUniforms(Shader* meshShader, const Scene& scene, const Camera3D& camera)
{
    glm::vec3 travel(0.35f, -0.85f, 0.25f);
    glm::vec3 lightRgb(1.f);
    float intensity = 1.f;

    const auto& reg = scene.GetRegistry();
    auto viewLights = reg.view<DirectionalLightComponent>();
    for (auto e : viewLights)
    {
        const auto& L = viewLights.get<DirectionalLightComponent>(e);
        travel = L.direction;
        lightRgb = L.color;
        intensity = L.intensity;
        break;
    }

    if (glm::length(travel) > 1e-6f)
        travel = glm::normalize(travel);
    else
        travel = glm::normalize(glm::vec3(0.35f, -0.85f, 0.25f));

    const glm::vec3 towardLight = -travel;
    glm::vec3 lightColor = lightRgb * glm::max(intensity, 0.f);
    // Avoid pitch-black meshes when the light color or intensity was zeroed in the inspector.
    if (glm::length(lightColor) < 0.02f)
        lightColor = glm::vec3(1.f);
    constexpr glm::vec3 kAmbient(0.18f, 0.19f, 0.22f);

    meshShader->SetVec3("u_CameraPos", camera.GetPosition());
    meshShader->SetVec3("u_LightDirToSurface", towardLight);
    meshShader->SetVec3("u_LightColor", lightColor);
    meshShader->SetVec3("u_Ambient", kAmbient);
}

} // namespace

Scene3D::Scene3D(const std::string& name) : Scene(name) {}

bool Scene3D::HasMesh3DDrawables(const Scene& scene)
{
    auto viewMeshes = scene.GetRegistry().view<Mesh3DComponent, Transform3DComponent>();
    return viewMeshes.begin() != viewMeshes.end();
}

bool Scene3D::HasSkyboxConfigured(const Scene& scene)
{
    const auto& reg = scene.GetRegistry();
    auto view = reg.view<SkyboxComponent>();
    for (auto e : view)
    {
        if (!view.get<SkyboxComponent>(e).crossTextureAsset.empty())
            return true;
    }
    return false;
}

void Scene3D::RenderMesh3DEntities(Scene& scene, const Camera3D& camera, uint32_t viewportWidth,
                                  uint32_t viewportHeight, AssetManager* assets)
{
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix(viewportWidth, viewportHeight);

    const GLboolean cullWas = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    Shader* meshShader = Renderer::GetMesh3DShader();
    if (!meshShader)
    {
        FDE_LOG_CLIENT_ERROR("Scene3D: Mesh3D shader unavailable");
        if (cullWas)
            glEnable(GL_CULL_FACE);
        return;
    }
    Renderer::SetShader(meshShader);
    ApplyMesh3DLightingUniforms(meshShader, scene, camera);

    auto viewMeshes = scene.GetRegistry().view<Mesh3DComponent, Transform3DComponent>();
    for (auto entity : viewMeshes)
    {
        auto& mesh = viewMeshes.get<Mesh3DComponent>(entity);
        auto& transform = viewMeshes.get<Transform3DComponent>(entity);

        if (!mesh.vertexArray || mesh.vertexArray->GetIndexCount() == 0)
            continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.position);
        model = glm::rotate(model, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, transform.scale);

        Renderer::SetMVP(model, view, projection);
        meshShader->SetMat4("u_Model", model);

        const bool hasTex = assets && mesh.albedoTexture && mesh.albedoTexture->glTextureId != 0;
        meshShader->SetInt("u_UseTexture", hasTex ? 1 : 0);
        if (hasTex)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(mesh.albedoTexture->glTextureId));
            meshShader->SetInt("u_Tex", 0);
        }

        Renderer::DrawIndexed(mesh.vertexArray);
    }

    Renderer::UseDefaultShader();
    glBindTexture(GL_TEXTURE_2D, 0);

    if (cullWas)
        glEnable(GL_CULL_FACE);
}

void Scene3D::RenderSkyboxIfAny(Scene& scene, const Camera3D& camera, uint32_t viewportWidth,
                                uint32_t viewportHeight, AssetManager* assets)
{
    if (viewportWidth == 0 || viewportHeight == 0)
        return;

    auto& reg = scene.GetRegistry();
    auto viewSky = reg.view<SkyboxComponent>();
    for (auto entity : viewSky)
    {
        auto& sb = viewSky.get<SkyboxComponent>(entity);
        if (assets)
            assets->ResolveSkybox(sb);
        if (!sb.cubemap || sb.cubemap->glTextureId == 0)
            continue;

        const glm::mat4 viewMat = camera.GetViewMatrix();
        const glm::mat4 proj = camera.GetProjectionMatrix(viewportWidth, viewportHeight);
        Renderer::DrawSkybox(sb.cubemap->glTextureId, viewMat, proj);
        break;
    }
}

void Scene3D::Render(const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                     AssetManager* assets)
{
    RenderSkyboxIfAny(*this, camera, viewportWidth, viewportHeight, assets);
    RenderMesh3DEntities(*this, camera, viewportWidth, viewportHeight, assets);
}

} // namespace FDE
