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

Scene3D::Scene3D(const std::string& name) : Scene(name) {}

bool Scene3D::HasMesh3DDrawables(const Scene& scene)
{
    auto viewMeshes = scene.GetRegistry().view<Mesh3DComponent, Transform3DComponent>();
    return viewMeshes.begin() != viewMeshes.end();
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

void Scene3D::Render(const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                     AssetManager* assets)
{
    RenderMesh3DEntities(*this, camera, viewportWidth, viewportHeight, assets);
}

} // namespace FDE
