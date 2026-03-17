#include "FDE/Renderer/OpenGL/OpenGLRendererAPI.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include <glad/glad.h>

namespace FDE
{

void OpenGLRendererAPI::Init()
{
}

void OpenGLRendererAPI::Shutdown()
{
}

void OpenGLRendererAPI::SetClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void OpenGLRendererAPI::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererAPI::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    if (!vertexArray)
        return;

    uint32_t count = indexCount > 0 ? indexCount : vertexArray->GetIndexCount();
    if (count == 0)
        return;

    vertexArray->Bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(count), GL_UNSIGNED_INT, nullptr);
}

void OpenGLRendererAPI::DrawTriangles(uint32_t vertexCount)
{
    if (vertexCount == 0)
        return;
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

} // namespace FDE
