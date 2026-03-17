#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/Viewport.hpp"
#include <cstdint>

namespace FDE
{

class FDE_API OpenGLViewport : public Viewport
{
  public:
    OpenGLViewport(uint32_t width, uint32_t height);
    ~OpenGLViewport() override;

    void Resize(uint32_t width, uint32_t height) override;
    void Bind() override;
    void Unbind() override;

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    void* GetColorAttachmentTextureId() const override;

    bool IsValid() const override { return m_framebufferId != 0; }

  private:
    void Invalidate();

    uint32_t m_framebufferId = 0;
    uint32_t m_colorAttachmentId = 0;
    uint32_t m_depthAttachmentId = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace FDE
