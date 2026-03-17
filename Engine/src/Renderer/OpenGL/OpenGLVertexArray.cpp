#include "FDE/Renderer/OpenGL/OpenGLVertexArray.hpp"
#include "FDE/Renderer/OpenGL/OpenGLVertexBuffer.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
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

OpenGLVertexArray::OpenGLVertexArray()
{
    glGenVertexArrays(1, &m_id);
}

OpenGLVertexArray::~OpenGLVertexArray()
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

void OpenGLVertexArray::Bind() const
{
    glBindVertexArray(m_id);
}

void OpenGLVertexArray::Unbind() const
{
    glBindVertexArray(0);
}

void OpenGLVertexArray::AddVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer, const BufferLayout& layout)
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

void OpenGLVertexArray::SetIndexBuffer(const uint32_t* indices, uint32_t count)
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
