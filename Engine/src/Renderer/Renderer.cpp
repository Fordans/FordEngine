#include "FDE/pch.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Renderer/VertexArray.hpp"
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

const char* s_mesh3DVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Color;
layout(location = 3) in vec2 a_TexCoord;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec3 v_Color;
out vec2 v_TexCoord;

void main()
{
    vec4 wp = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = wp.xyz;
    mat3 nm = transpose(inverse(mat3(u_Model)));
    v_Normal = nm * a_Normal;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
}
)";

const char* s_mesh3DFragmentShader = R"(
#version 330 core
layout(location = 0) out vec4 o_Color;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec3 v_Color;
in vec2 v_TexCoord;

uniform int u_UseTexture;
uniform sampler2D u_Tex;

uniform vec3 u_CameraPos;
uniform vec3 u_LightDirToSurface;
uniform vec3 u_LightColor;
uniform vec3 u_Ambient;

void main()
{
    vec3 albedo = v_Color;
    if (u_UseTexture != 0)
    {
        vec3 samp = texture(u_Tex, v_TexCoord).rgb;
        // Dark / empty albedo maps should not wipe out vertex tint entirely (was often read as "no lighting").
        samp = max(samp, vec3(0.15));
        albedo *= samp;
    }

    float nn = dot(v_Normal, v_Normal);
    vec3 N = (nn > 1e-8) ? normalize(v_Normal) : vec3(0.0, 1.0, 0.0);
    vec3 L = normalize(u_LightDirToSurface);
    float ndl = max(dot(N, L), 0.0);
    // Soften terminator so faces never go fully black from directional alone.
    ndl = mix(0.35, 1.0, ndl);
    vec3 diffuse = u_LightColor * ndl * albedo;

    vec3 V = normalize(u_CameraPos - v_WorldPos);
    vec3 H = normalize(V + L);
    float spec = pow(max(dot(N, H), 0.0), 48.0) * 0.2;
    vec3 specular = u_LightColor * spec;

    vec3 lit = u_Ambient * albedo + diffuse + specular;
    lit += vec3(0.03);
    o_Color = vec4(lit, 1.0);
}
)";

const char* s_skyboxVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProj;

out vec3 v_TexDir;

void main()
{
    v_TexDir = a_Position;
    vec4 clip = u_ViewProj * vec4(a_Position, 1.0);
    gl_Position = clip.xyww;
}
)";

const char* s_skyboxFragmentShader = R"(
#version 330 core
layout(location = 0) out vec4 o_Color;

in vec3 v_TexDir;

uniform samplerCube u_Skybox;

void main()
{
    o_Color = texture(u_Skybox, normalize(v_TexDir));
}
)";

// Unit cube, 36 vertices (position only); drawn from inside with culling off.
const float s_skyboxVerts[] = {
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
};

std::unique_ptr<Shader> s_defaultShader;
std::unique_ptr<Shader> s_simpleShader;
std::unique_ptr<Shader> s_mesh3DShader;
std::unique_ptr<Shader> s_skyboxShader;
GLuint s_skyboxVao = 0;
GLuint s_skyboxVbo = 0;
Shader* s_currentShader = nullptr;

} // namespace

void Renderer::Init(GraphicsAPI api)
{
    RenderCommand::Init(api);

    s_defaultShader = Shader::Create(s_defaultVertexShader, s_defaultFragmentShader);
    s_simpleShader = Shader::Create(s_simpleVertexShader, s_simpleFragmentShader);
    s_mesh3DShader = Shader::Create(s_mesh3DVertexShader, s_mesh3DFragmentShader);
    s_skyboxShader = Shader::Create(s_skyboxVertexShader, s_skyboxFragmentShader);
    s_currentShader = s_defaultShader.get();

    static_assert(sizeof(s_skyboxVerts) == sizeof(float) * 3u * 36u, "skybox vertex count");
    glGenVertexArrays(1, &s_skyboxVao);
    glGenBuffers(1, &s_skyboxVbo);
    glBindVertexArray(s_skyboxVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_skyboxVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_skyboxVerts), s_skyboxVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(float) * 3), nullptr);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::Shutdown()
{
    s_defaultShader.reset();
    s_simpleShader.reset();
    s_mesh3DShader.reset();
    s_skyboxShader.reset();
    s_currentShader = nullptr;

    if (s_skyboxVao)
    {
        glDeleteVertexArrays(1, &s_skyboxVao);
        s_skyboxVao = 0;
    }
    if (s_skyboxVbo)
    {
        glDeleteBuffers(1, &s_skyboxVbo);
        s_skyboxVbo = 0;
    }

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

void Renderer::DrawSkybox(unsigned int cubemapGlId, const glm::mat4& view, const glm::mat4& projection)
{
    if (cubemapGlId == 0 || !s_skyboxShader || s_skyboxVao == 0)
        return;

    const glm::mat4 viewRot = glm::mat4(glm::mat3(view));
    const glm::mat4 viewProj = projection * viewRot;

    GLint depthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    GLboolean depthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    const GLboolean cullWas = glIsEnabled(GL_CULL_FACE);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    s_skyboxShader->Bind();
    s_skyboxShader->SetMat4("u_ViewProj", viewProj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cubemapGlId));
    s_skyboxShader->SetInt("u_Skybox", 0);

    glBindVertexArray(s_skyboxVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    if (cullWas)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    glDepthMask(depthMask);
    glDepthFunc(depthFunc);
}

} // namespace FDE
