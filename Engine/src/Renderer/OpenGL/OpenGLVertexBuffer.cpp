#include "FDE/pch.hpp"
#include "FDE/Renderer/OpenGL/OpenGLVertexBuffer.hpp"
#include <glad/glad.h>

namespace FDE
{

OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, size_t size)
{
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
    if (m_id)
    {
        glDeleteBuffers(1, &m_id);
    }
}

void OpenGLVertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
}

void OpenGLVertexBuffer::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLVertexBuffer::SetData(const void* data, size_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size), data);
}

} // namespace FDE
