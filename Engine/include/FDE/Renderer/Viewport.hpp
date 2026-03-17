#pragma once

#include "FDE/Export.hpp"
#include <cstdint>
#include <memory>

namespace FDE
{

/// Abstract interface for framebuffer/viewport resources.
/// Use Viewport::Create() to obtain a backend-specific implementation.
class FDE_API Viewport
{
  public:
    virtual ~Viewport() = default;

    virtual void Resize(uint32_t width, uint32_t height) = 0;
    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    /// Backend-specific texture ID for ImGui/UI (e.g. OpenGL texture ID).
    virtual void* GetColorAttachmentTextureId() const = 0;

    virtual bool IsValid() const = 0;

    /// Factory: creates a viewport for the active graphics API.
    static std::unique_ptr<Viewport> Create(uint32_t width, uint32_t height);

  protected:
    Viewport() = default;
};

} // namespace FDE
