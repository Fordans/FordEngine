#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include <cstdint>
#include <vector>

namespace FDE
{

class FDE_API OpenGLVertexArray : public VertexArray
{
  public:
    OpenGLVertexArray();
    ~OpenGLVertexArray() override;

    void Bind() const override;
    void Unbind() const override;

    void AddVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer, const BufferLayout& layout) override;
    void SetIndexBuffer(const uint32_t* indices, uint32_t count) override;

    const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const override { return m_vertexBuffers; }
    uint32_t GetIndexCount() const override { return m_indexCount; }

  private:
    uint32_t m_id = 0;
    uint32_t m_vertexBufferIndex = 0;
    uint32_t m_indexBufferId = 0;
    uint32_t m_indexCount = 0;
    std::vector<std::shared_ptr<VertexBuffer>> m_vertexBuffers;
};

} // namespace FDE
