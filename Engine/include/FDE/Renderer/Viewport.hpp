#pragma once

#include "FDE/Export.hpp"
#include <cstdint>

namespace FDE
{

class FDE_API Viewport
{
  public:
    Viewport(uint32_t width, uint32_t height);
    ~Viewport();

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    void Resize(uint32_t width, uint32_t height);

    void Bind();
    void Unbind();

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetColorAttachmentId() const { return m_colorAttachmentId; }
    void* GetColorAttachmentTextureId() const;

    bool IsValid() const { return m_framebufferId != 0; }

  private:
    void Invalidate();

    uint32_t m_framebufferId = 0;
    uint32_t m_colorAttachmentId = 0;
    uint32_t m_depthAttachmentId = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace FDE
