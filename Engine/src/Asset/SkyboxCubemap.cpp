#include "FDE/pch.hpp"
#include "FDE/Asset/CubemapResource.hpp"
#include "FDE/Core/Log.hpp"
#include <glad/glad.h>
#include <stb_image.h>
#include <cstring>
#include <memory>
#include <vector>

namespace FDE
{

namespace
{

/// stb: row 0 = top of PNG. glTexImage2D: first row of buffer = bottom of texture (t=0).
void CopyFaceBottomRowFirst(const unsigned char* src, int srcW, int ox, int oy, int faceSize,
                            std::vector<unsigned char>& faceRgba)
{
    faceRgba.resize(static_cast<size_t>(faceSize) * static_cast<size_t>(faceSize) * 4u);
    for (int row = 0; row < faceSize; ++row)
    {
        const int srcY = oy + (faceSize - 1 - row);
        const unsigned char* srcRow = src + (static_cast<ptrdiff_t>(srcY) * srcW + ox) * 4;
        unsigned char* dstRow = faceRgba.data() + static_cast<ptrdiff_t>(row) * faceSize * 4;
        std::memcpy(dstRow, srcRow, static_cast<size_t>(faceSize) * 4u);
    }
}

} // namespace

bool LoadCubemapFromHorizontalCrossPng(const char* absolutePath, std::shared_ptr<CubemapResource>& outCubemap)
{
    outCubemap.reset();
    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = stbi_load(absolutePath, &w, &h, &ch, 4);
    if (!pixels || w <= 0 || h <= 0)
    {
        if (pixels)
            stbi_image_free(pixels);
        FDE_LOG_CLIENT_ERROR("Skybox: failed to load image {}", absolutePath ? absolutePath : "");
        return false;
    }

    const int faceW = w / 4;
    const int faceH = h / 3;
    if (faceW != faceH || faceW <= 0)
    {
        stbi_image_free(pixels);
        FDE_LOG_CLIENT_ERROR("Skybox: expected 4:3 horizontal cross with square faces ({})", absolutePath);
        return false;
    }
    const int s = faceW;

    // Layout: row0 [ _ +Y _ _ ], row1 [ -X +Z +X -Z ], row2 [ _ -Y _ _ ]
    struct FaceSlot
    {
        int col, row;
    };
    const FaceSlot slots[6] = {
        {2, 1}, // +X
        {0, 1}, // -X
        {1, 0}, // cross top cell → GL −Y (swapped vs cross bottom for this asset pipeline)
        {1, 2}, // cross bottom cell → GL +Y
        {1, 1}, // +Z
        {3, 1}, // -Z
    };

    const GLenum targets[6] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::vector<unsigned char> faceBuf;
    for (int i = 0; i < 6; ++i)
    {
        const int ox = slots[i].col * s;
        const int oy = slots[i].row * s;
        CopyFaceBottomRowFirst(pixels, w, ox, oy, s, faceBuf);
        // Caps had a single in-plane 180°; “再转 180°” cancels it (same as no rotation here).
        glTexImage2D(targets[i], 0, GL_RGBA, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, faceBuf.data());
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    stbi_image_free(pixels);

    outCubemap = std::make_shared<CubemapResource>();
    outCubemap->glTextureId = tex;
    return true;
}

} // namespace FDE
