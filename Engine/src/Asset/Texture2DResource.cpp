#include "FDE/pch.hpp"
#include "FDE/Asset/Texture2DResource.hpp"
#include <glad/glad.h>

namespace FDE
{

Texture2DResource::~Texture2DResource()
{
    if (glTextureId != 0)
    {
        GLuint t = static_cast<GLuint>(glTextureId);
        glDeleteTextures(1, &t);
        glTextureId = 0;
    }
}

} // namespace FDE
