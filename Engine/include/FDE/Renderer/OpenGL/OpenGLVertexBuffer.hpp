#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"
#include <cstdint>

namespace FDE
{

class FDE_API OpenGLVertexBuffer : public VertexBuffer
{
  public:
    OpenGLVertexBuffer(const void* data, size_t size);
    ~OpenGLVertexBuffer() override;

    void Bind() const override;
    void Unbind() const override;
    void SetData(const void* data, size_t size) override;

  private:
    uint32_t m_id = 0;
};

} // namespace FDE
