#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace FDE
{

class FDE_API Renderer
{
  public:
    static void Init();
    static void Shutdown();

    static void SetClearColor(float r, float g, float b, float a);
    static void Clear();

    static void SetViewport(int x, int y, int width, int height);

    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
    static void DrawTriangles(uint32_t vertexCount);

    static void SetMVP(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
    static void SetShader(Shader* shader);
};

} // namespace FDE
