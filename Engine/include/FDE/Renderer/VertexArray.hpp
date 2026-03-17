#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"
#include <memory>
#include <vector>

namespace FDE
{

class FDE_API VertexArray
{
  public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer, const BufferLayout& layout);
    void SetIndexBuffer(const uint32_t* indices, uint32_t count);

    const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const { return m_vertexBuffers; }
    uint32_t GetIndexCount() const { return m_indexCount; }

    uint32_t GetId() const { return m_id; }

  private:
    uint32_t m_id = 0;
    uint32_t m_vertexBufferIndex = 0;
    uint32_t m_indexBufferId = 0;
    uint32_t m_indexCount = 0;
    std::vector<std::shared_ptr<VertexBuffer>> m_vertexBuffers;
};

} // namespace FDE
