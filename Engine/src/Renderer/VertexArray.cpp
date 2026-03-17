#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Renderer/OpenGL/OpenGLVertexArray.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Core/Log.hpp"
#include <memory>

namespace FDE
{

std::shared_ptr<VertexArray> VertexArray::Create()
{
    switch (RenderCommand::GetAPI())
    {
    case GraphicsAPI::OpenGL:
        return std::make_shared<OpenGLVertexArray>();
    default:
        FDE_LOG_CLIENT_ERROR("Unsupported graphics API for VertexArray creation");
        return nullptr;
    }
}

} // namespace FDE
