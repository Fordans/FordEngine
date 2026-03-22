#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace FDE
{

class VertexBuffer;

/// Abstract vertex array (VAO + VBOs + index buffer).
class FDE_API VertexArray
{
  public:
    virtual ~VertexArray() = default;

    static std::shared_ptr<VertexArray> Create();

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual void AddVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer, const BufferLayout& layout) = 0;
    virtual void SetIndexBuffer(const uint32_t* indices, uint32_t count) = 0;

    virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const = 0;
    virtual uint32_t GetIndexCount() const = 0;

  protected:
    VertexArray() = default;
};

} // namespace FDE
