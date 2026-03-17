#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/IRendererAPI.hpp"

namespace FDE
{

class FDE_API OpenGLRendererAPI : public IRendererAPI
{
  public:
    GraphicsAPI GetAPI() const override { return GraphicsAPI::OpenGL; }

    void Init() override;
    void Shutdown() override;

    void SetClearColor(float r, float g, float b, float a) override;
    void Clear() override;
    void SetViewport(int x, int y, int width, int height) override;

    void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
    void DrawTriangles(uint32_t vertexCount) override;
};

} // namespace FDE
