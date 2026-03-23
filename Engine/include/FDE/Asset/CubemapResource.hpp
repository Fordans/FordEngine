#pragma once

#include "FDE/Export.hpp"

namespace FDE
{

/// OpenGL cubemap texture name (GL_TEXTURE_CUBE_MAP).
struct FDE_API CubemapResource
{
    unsigned int glTextureId = 0;
    ~CubemapResource();
};

} // namespace FDE
