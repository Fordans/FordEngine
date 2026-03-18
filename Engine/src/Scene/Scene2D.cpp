#include "FDE/pch.hpp"
#include "FDE/Scene/Scene2D.hpp"

namespace FDE
{

Scene2D::Scene2D(const std::string& name) : Scene(name) {}

void Scene2D::Render(const Camera2D& camera, uint32_t viewportWidth, uint32_t viewportHeight)
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = camera.GetViewProjectionMatrix(viewportWidth, viewportHeight);

    auto view2D = GetRegistry().view<Mesh2DComponent, Transform2DComponent>();
    for (auto entity : view2D)
    {
        auto& mesh = view2D.get<Mesh2DComponent>(entity);
        auto& transform = view2D.get<Transform2DComponent>(entity);

        if (!mesh.vertexArray || mesh.vertexArray->GetIndexCount() == 0)
            continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(transform.position, 0.0f));
        model = glm::rotate(model, transform.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(transform.scale, 1.0f));

        Renderer::SetMVP(model, view, projection);
        Renderer::DrawIndexed(mesh.vertexArray);
    }
}

} // namespace FDE
