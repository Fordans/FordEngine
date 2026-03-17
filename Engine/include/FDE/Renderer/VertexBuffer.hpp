#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include <cstddef>
#include <memory>

namespace FDE
{

/// Abstract interface for vertex buffer resources.
/// Use VertexBuffer::Create() to obtain a backend-specific implementation.
class FDE_API VertexBuffer
{
  public:
    virtual ~VertexBuffer() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
    virtual void SetData(const void* data, size_t size) = 0;

    /// Factory: creates a vertex buffer for the active graphics API.
    static std::shared_ptr<VertexBuffer> Create(const void* data, size_t size);

  protected:
    VertexBuffer() = default;
};

} // namespace FDE
