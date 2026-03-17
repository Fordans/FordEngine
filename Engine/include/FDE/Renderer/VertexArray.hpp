#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"
#include <memory>
#include <vector>

namespace FDE
{

/// Abstract interface for vertex array (VAO) resources.
/// Use VertexArray::Create() to obtain a backend-specific implementation.
class FDE_API VertexArray
{
  public:
    virtual ~VertexArray() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual void AddVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer, const BufferLayout& layout) = 0;
    virtual void SetIndexBuffer(const uint32_t* indices, uint32_t count) = 0;

    virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const = 0;
    virtual uint32_t GetIndexCount() const = 0;

    /// Factory: creates a vertex array for the active graphics API.
    static std::shared_ptr<VertexArray> Create();

  protected:
    VertexArray() = default;
};

} // namespace FDE
