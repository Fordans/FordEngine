#include "FDE/pch.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Renderer/OpenGL/OpenGLShader.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Core/Log.hpp"
#include <memory>

namespace FDE
{

std::unique_ptr<Shader> Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    switch (RenderCommand::GetAPI())
    {
    case GraphicsAPI::OpenGL:
        return std::make_unique<OpenGLShader>(vertexSrc, fragmentSrc);
    default:
        FDE_LOG_CLIENT_ERROR("Unsupported graphics API for Shader creation");
        return nullptr;
    }
}

} // namespace FDE
