#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Renderer/IRendererAPI.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include <memory>

namespace FDE
{

/// Static command buffer that delegates to the active graphics API backend.
/// Use this to perform render operations without coupling to a specific API.
class FDE_API RenderCommand
{
  public:
    static void Init(GraphicsAPI api = GraphicsAPI::OpenGL);
    static void Shutdown();

    static GraphicsAPI GetAPI();
    static IRendererAPI* GetRendererAPI();

    static void SetClearColor(float r, float g, float b, float a);
    static void Clear();
    static void SetViewport(int x, int y, int width, int height);
    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
    static void DrawTriangles(uint32_t vertexCount);

  private:
    static std::unique_ptr<IRendererAPI> s_rendererAPI;
};

} // namespace FDE
