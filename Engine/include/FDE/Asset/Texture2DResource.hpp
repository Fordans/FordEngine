#pragma once

#include "FDE/Export.hpp"

namespace FDE
{

/// GPU texture handle + dimensions (OpenGL texture name).
struct FDE_API Texture2DResource
{
    unsigned int glTextureId = 0;
    int width = 0;
    int height = 0;
    ~Texture2DResource();
};

} // namespace FDE
