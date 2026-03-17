#include "FDE/Renderer/VertexArray.hpp"
#include <glad/glad.h>

namespace FDE
{

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
{
    switch (type)
    {
    case ShaderDataType::Float:
    case ShaderDataType::Float2:
    case ShaderDataType::Float3:
    case ShaderDataType::Float4:
        return GL_FLOAT;
    case ShaderDataType::Int:
    case ShaderDataType::Int2:
    case ShaderDataType::Int3:
    case ShaderDataType::Int4:
        return GL_INT;
    case ShaderDataType::Mat3:
    case ShaderDataType::Mat4:
        return GL_FLOAT;
    default:
        return GL_FLOAT;
    }
}

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray()
{
    if (m_id)
    {
        glDeleteVertexArrays(1, &m_id);
    }
    if (m_indexBufferId)
    {
        glDeleteBuffers(1, &m_indexBufferId);
    }
}

void VertexArray::Bind() const
{
    glBindVertexArray(m_id);
}

void VertexArray::Unbind() const
{
    glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer, const BufferLayout& layout)
{
    glBindVertexArray(m_id);
    vertexBuffer->Bind();

    for (const auto& element : layout)
    {
        glEnableVertexAttribArray(m_vertexBufferIndex);
        glVertexAttribPointer(m_vertexBufferIndex, element.GetComponentCount(),
                             ShaderDataTypeToOpenGLBaseType(element.type), element.normalized ? GL_TRUE : GL_FALSE,
                             static_cast<GLsizei>(layout.GetStride()),
                             reinterpret_cast<const void*>(static_cast<uintptr_t>(element.offset)));
        m_vertexBufferIndex++;
    }

    m_vertexBuffers.push_back(std::move(vertexBuffer));
}

void VertexArray::SetIndexBuffer(const uint32_t* indices, uint32_t count)
{
    glBindVertexArray(m_id);
    if (m_indexBufferId)
        glDeleteBuffers(1, &m_indexBufferId);
    glGenBuffers(1, &m_indexBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    m_indexCount = count;
}

} // namespace FDE
