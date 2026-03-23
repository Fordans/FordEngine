#include "FDE/pch.hpp"
#include "FDE/Scene/Scene3D.hpp"
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
                                  uint32_t viewportHeight)
{
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix(viewportWidth, viewportHeight);

    Shader* mesh3dShader = Renderer::GetMesh3DShader();
    if (mesh3dShader)
        Renderer::SetShader(mesh3dShader);

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

        if (mesh3dShader)
        {
            const bool useTex = mesh.albedoTexture && mesh.albedoTexture->glTextureId != 0;
            mesh3dShader->SetInt("u_UseTexture", useTex ? 1 : 0);
            if (useTex)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(mesh.albedoTexture->glTextureId));
                mesh3dShader->SetInt("u_Albedo", 0);
            }
        }

        Renderer::DrawIndexed(mesh.vertexArray);

        if (mesh3dShader && mesh.albedoTexture && mesh.albedoTexture->glTextureId != 0)
            glBindTexture(GL_TEXTURE_2D, 0);
    }

    Renderer::UseDefaultShader();
}

void Scene3D::Render(const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight)
{
    RenderMesh3DEntities(*this, camera, viewportWidth, viewportHeight);
}

} // namespace FDE
