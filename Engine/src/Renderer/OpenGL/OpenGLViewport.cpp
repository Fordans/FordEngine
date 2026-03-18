#include "FDE/pch.hpp"
#include "FDE/Renderer/OpenGL/OpenGLViewport.hpp"
#include "FDE/Core/Log.hpp"
#include <glad/glad.h>

namespace FDE
{

OpenGLViewport::OpenGLViewport(uint32_t width, uint32_t height) : m_width(width), m_height(height)
{
    Invalidate();
}

OpenGLViewport::~OpenGLViewport()
{
    if (m_framebufferId)
    {
        glDeleteFramebuffers(1, &m_framebufferId);
        m_framebufferId = 0;
    }
    if (m_colorAttachmentId)
    {
        glDeleteTextures(1, &m_colorAttachmentId);
        m_colorAttachmentId = 0;
    }
    if (m_depthAttachmentId)
    {
        glDeleteRenderbuffers(1, &m_depthAttachmentId);
        m_depthAttachmentId = 0;
    }
}

void OpenGLViewport::Invalidate()
{
    if (m_width == 0 || m_height == 0)
        return;

    if (m_framebufferId)
    {
        glDeleteFramebuffers(1, &m_framebufferId);
        m_framebufferId = 0;
    }
    if (m_colorAttachmentId)
    {
        glDeleteTextures(1, &m_colorAttachmentId);
        m_colorAttachmentId = 0;
    }
    if (m_depthAttachmentId)
    {
        glDeleteRenderbuffers(1, &m_depthAttachmentId);
        m_depthAttachmentId = 0;
    }

    glGenFramebuffers(1, &m_framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

    glGenTextures(1, &m_colorAttachmentId);
    glBindTexture(GL_TEXTURE_2D, m_colorAttachmentId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorAttachmentId, 0);

    glGenRenderbuffers(1, &m_depthAttachmentId);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthAttachmentId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast<GLsizei>(m_width),
                          static_cast<GLsizei>(m_height));
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthAttachmentId);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        FDE_LOG_CLIENT_ERROR("Viewport framebuffer is incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLViewport::Resize(uint32_t width, uint32_t height)
{
    if (m_width == width && m_height == height)
        return;
    m_width = width;
    m_height = height;
    Invalidate();
}

void OpenGLViewport::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    glViewport(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));
}

void OpenGLViewport::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void* OpenGLViewport::GetColorAttachmentTextureId() const
{
    return reinterpret_cast<void*>(static_cast<intptr_t>(m_colorAttachmentId));
}

} // namespace FDE
