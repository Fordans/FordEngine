#include "FDE/Renderer/Viewport.hpp"
#include "FDE/Renderer/OpenGL/OpenGLViewport.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Core/Log.hpp"
#include <memory>

namespace FDE
{

std::unique_ptr<Viewport> Viewport::Create(uint32_t width, uint32_t height)
{
    switch (RenderCommand::GetAPI())
    {
    case GraphicsAPI::OpenGL:
        return std::make_unique<OpenGLViewport>(width, height);
    default:
        FDE_LOG_CLIENT_ERROR("Unsupported graphics API for Viewport creation");
        return nullptr;
    }
}

} // namespace FDE
