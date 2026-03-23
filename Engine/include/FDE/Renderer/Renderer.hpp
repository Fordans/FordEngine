#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/GraphicsAPI.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace FDE
{

class FDE_API Renderer
{
  public:
    /// Initialize the renderer with the given graphics API.
    /// Defaults to OpenGL. Call before creating any render resources.
    static void Init(GraphicsAPI api = GraphicsAPI::OpenGL);
    static void Shutdown();

    static void SetClearColor(float r, float g, float b, float a);
    static void Clear();

    static void SetViewport(int x, int y, int width, int height);

    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
    static void DrawTriangles(uint32_t vertexCount);
    static void DrawLines(uint32_t vertexCount);

    static void SetMVP(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
    static void SetShader(Shader* shader);
    /// Returns the simple shader (position-only, u_Color uniform) for lines, debug draw, etc.
    static Shader* GetSimpleShader();
    /// Position + color + UV; optional \p u_UseTexture + \p u_Albedo (unit 0). Used for Mesh3D.
    static Shader* GetMesh3DShader();
    /// Restores the default shader (for colored meshes). Call after using GetSimpleShader() or GetMesh3DShader().
    static void UseDefaultShader();
};

} // namespace FDE
