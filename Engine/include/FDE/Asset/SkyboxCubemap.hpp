#pragma once

#include "FDE/Export.hpp"
#include <memory>

namespace FDE
{

struct CubemapResource;

/// Loads a 4×3 horizontal-cross cubemap layout (PNG). Each cell must be square.
bool FDE_API LoadCubemapFromHorizontalCrossPng(const char* absolutePath, std::shared_ptr<CubemapResource>& outCubemap);

} // namespace FDE
