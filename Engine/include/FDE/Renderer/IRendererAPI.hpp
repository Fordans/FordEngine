#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include <memory>

namespace FDE
{

/// Abstract interface for low-level graphics API operations.
/// Implementations: OpenGLRendererAPI, (future: VulkanRendererAPI, etc.)
class FDE_API IRendererAPI
{
  public:
    virtual ~IRendererAPI() = default;

    virtual GraphicsAPI GetAPI() const = 0;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    virtual void SetClearColor(float r, float g, float b, float a) = 0;
    virtual void Clear() = 0;

    virtual void SetViewport(int x, int y, int width, int height) = 0;

    virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
    virtual void DrawTriangles(uint32_t vertexCount) = 0;
};

} // namespace FDE
