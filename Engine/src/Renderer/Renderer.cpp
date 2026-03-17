#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/Shader.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace FDE
{

namespace
{

const char* s_defaultVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;

uniform mat4 u_MVP;

out vec3 v_Color;

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
    v_Color = a_Color;
}
)";

const char* s_defaultFragmentShader = R"(
#version 330 core
layout(location = 0) out vec4 o_Color;

in vec3 v_Color;

void main()
{
    o_Color = vec4(v_Color, 1.0);
}
)";

const char* s_simpleVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_MVP;

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

const char* s_simpleFragmentShader = R"(
#version 330 core
layout(location = 0) out vec4 o_Color;

uniform vec4 u_Color;

void main()
{
    o_Color = u_Color;
}
)";

std::unique_ptr<Shader> s_defaultShader;
std::unique_ptr<Shader> s_simpleShader;
Shader* s_currentShader = nullptr;

} // namespace

void Renderer::Init()
{
    s_defaultShader = std::make_unique<Shader>(s_defaultVertexShader, s_defaultFragmentShader);
    s_simpleShader = std::make_unique<Shader>(s_simpleVertexShader, s_simpleFragmentShader);
    s_currentShader = s_defaultShader.get();
}

void Renderer::Shutdown()
{
    s_defaultShader.reset();
    s_simpleShader.reset();
    s_currentShader = nullptr;
}

void Renderer::SetClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void Renderer::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void Renderer::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    if (!vertexArray || !s_currentShader)
        return;

    uint32_t count = indexCount > 0 ? indexCount : vertexArray->GetIndexCount();
    if (count == 0)
        return;

    s_currentShader->Bind();
    vertexArray->Bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(count), GL_UNSIGNED_INT, nullptr);
}

void Renderer::DrawTriangles(uint32_t vertexCount)
{
    if (!s_currentShader || vertexCount == 0)
        return;

    s_currentShader->Bind();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

void Renderer::SetMVP(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
{
    if (s_currentShader)
    {
        glm::mat4 mvp = projection * view * model;
        s_currentShader->SetMat4("u_MVP", mvp);
    }
}

void Renderer::SetShader(Shader* shader)
{
    s_currentShader = shader;
}

} // namespace FDE
