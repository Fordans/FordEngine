#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/OpenGL/OpenGLRendererAPI.hpp"
#include "FDE/Core/Log.hpp"
#include <memory>

namespace FDE
{

std::unique_ptr<IRendererAPI> RenderCommand::s_rendererAPI;

void RenderCommand::Init(GraphicsAPI api)
{
    switch (api)
    {
    case GraphicsAPI::OpenGL:
        s_rendererAPI = std::make_unique<OpenGLRendererAPI>();
        break;
    case GraphicsAPI::None:
        FDE_LOG_CLIENT_ERROR("GraphicsAPI::None is not a valid renderer backend");
        return;
    default:
        FDE_LOG_CLIENT_ERROR("Unsupported graphics API, falling back to OpenGL");
        s_rendererAPI = std::make_unique<OpenGLRendererAPI>();
        break;
    }
    s_rendererAPI->Init();
}

void RenderCommand::Shutdown()
{
    if (s_rendererAPI)
    {
        s_rendererAPI->Shutdown();
        s_rendererAPI.reset();
    }
}

GraphicsAPI RenderCommand::GetAPI()
{
    return s_rendererAPI ? s_rendererAPI->GetAPI() : GraphicsAPI::None;
}

IRendererAPI* RenderCommand::GetRendererAPI()
{
    return s_rendererAPI.get();
}

void RenderCommand::SetClearColor(float r, float g, float b, float a)
{
    if (s_rendererAPI)
        s_rendererAPI->SetClearColor(r, g, b, a);
}

void RenderCommand::Clear()
{
    if (s_rendererAPI)
        s_rendererAPI->Clear();
}

void RenderCommand::SetViewport(int x, int y, int width, int height)
{
    if (s_rendererAPI)
        s_rendererAPI->SetViewport(x, y, width, height);
}

void RenderCommand::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    if (s_rendererAPI)
        s_rendererAPI->DrawIndexed(vertexArray, indexCount);
}

void RenderCommand::DrawTriangles(uint32_t vertexCount)
{
    if (s_rendererAPI)
        s_rendererAPI->DrawTriangles(vertexCount);
}

} // namespace FDE
