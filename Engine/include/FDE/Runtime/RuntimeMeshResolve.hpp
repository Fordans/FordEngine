#pragma once

#include "FDE/Export.hpp"

namespace FDE
{

class World;
class AssetManager;

/// Resolves pending Mesh2D/Mesh3D GPU data for every scene in \p world.
/// Shared by the editor and runtime player paths (no Editor-only UI dependencies).
void FDE_API ResolvePendingMeshes(World* world, AssetManager* assets);

} // namespace FDE
