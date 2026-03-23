#include "FDE/pch.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Renderer/VertexArray.hpp"
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

const char* s_mesh3DVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_MVP;

out vec3 v_Color;
out vec2 v_TexCoord;

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
}
)";

const char* s_mesh3DFragmentShader = R"(
#version 330 core
layout(location = 0) out vec4 o_Color;

in vec3 v_Color;
in vec2 v_TexCoord;

uniform int u_UseTexture;
uniform sampler2D u_Tex;

void main()
{
    if (u_UseTexture != 0)
        o_Color = texture(u_Tex, v_TexCoord) * vec4(v_Color, 1.0);
    else
        o_Color = vec4(v_Color, 1.0);
}
)";

std::unique_ptr<Shader> s_defaultShader;
std::unique_ptr<Shader> s_simpleShader;
std::unique_ptr<Shader> s_mesh3DShader;
Shader* s_currentShader = nullptr;

} // namespace

void Renderer::Init(GraphicsAPI api)
{
    RenderCommand::Init(api);

    s_defaultShader = Shader::Create(s_defaultVertexShader, s_defaultFragmentShader);
    s_simpleShader = Shader::Create(s_simpleVertexShader, s_simpleFragmentShader);
    s_mesh3DShader = Shader::Create(s_mesh3DVertexShader, s_mesh3DFragmentShader);
    s_currentShader = s_defaultShader.get();
}

void Renderer::Shutdown()
{
    s_defaultShader.reset();
    s_simpleShader.reset();
    s_mesh3DShader.reset();
    s_currentShader = nullptr;

    RenderCommand::Shutdown();
}

void Renderer::SetClearColor(float r, float g, float b, float a)
{
    RenderCommand::SetClearColor(r, g, b, a);
}

void Renderer::Clear()
{
    RenderCommand::Clear();
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    RenderCommand::SetViewport(x, y, width, height);
}

void Renderer::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    if (!vertexArray || !s_currentShader)
        return;

    s_currentShader->Bind();
    RenderCommand::DrawIndexed(vertexArray, indexCount);
}

void Renderer::DrawTriangles(uint32_t vertexCount)
{
    if (!s_currentShader || vertexCount == 0)
        return;

    s_currentShader->Bind();
    RenderCommand::DrawTriangles(vertexCount);
}

void Renderer::DrawLines(uint32_t vertexCount)
{
    if (!s_currentShader || vertexCount == 0)
        return;

    s_currentShader->Bind();
    RenderCommand::DrawLines(vertexCount);
}

void Renderer::SetMVP(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
{
    if (s_currentShader)
    {
        // glUniform* affects the active program; must match s_currentShader.
        s_currentShader->Bind();
        glm::mat4 mvp = projection * view * model;
        s_currentShader->SetMat4("u_MVP", mvp);
    }
}

void Renderer::SetShader(Shader* shader)
{
    s_currentShader = shader;
}

Shader* Renderer::GetSimpleShader()
{
    return s_simpleShader.get();
}

Shader* Renderer::GetMesh3DShader()
{
    return s_mesh3DShader.get();
}

void Renderer::UseDefaultShader()
{
    s_currentShader = s_defaultShader.get();
    if (s_currentShader)
        s_currentShader->Bind();
}

} // namespace FDE
