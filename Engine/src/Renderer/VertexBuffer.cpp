#include "FDE/Renderer/VertexBuffer.hpp"
#include "FDE/Renderer/OpenGL/OpenGLVertexBuffer.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Core/Log.hpp"
#include <memory>

namespace FDE
{

std::shared_ptr<VertexBuffer> VertexBuffer::Create(const void* data, size_t size)
{
    switch (RenderCommand::GetAPI())
    {
    case GraphicsAPI::OpenGL:
        return std::make_shared<OpenGLVertexBuffer>(data, size);
    default:
        FDE_LOG_CLIENT_ERROR("Unsupported graphics API for VertexBuffer creation");
        return nullptr;
    }
}

} // namespace FDE
