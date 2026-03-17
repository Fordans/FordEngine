#pragma once

#include "FDE/Export.hpp"

namespace FDE
{

/// Supported graphics API backends.
/// Used to select and initialize the appropriate renderer implementation.
enum class FDE_API GraphicsAPI
{
    None = 0,
    OpenGL,
    // Vulkan,
    // Direct3D11,
    // Direct3D12,
};

} // namespace FDE
