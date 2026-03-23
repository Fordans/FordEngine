#include "FDE/pch.hpp"
#include "FDE/Asset/CubemapResource.hpp"
#include <glad/glad.h>

namespace FDE
{

CubemapResource::~CubemapResource()
{
    if (glTextureId != 0)
    {
        GLuint t = static_cast<GLuint>(glTextureId);
        glDeleteTextures(1, &t);
        glTextureId = 0;
    }
}

} // namespace FDE
