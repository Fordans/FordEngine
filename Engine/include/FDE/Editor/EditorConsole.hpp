#pragma once

#include "FDE/Export.hpp"
#include "imgui.h"

namespace FDE
{

class FDE_API EditorConsole
{
  public:
    static void Initialize();
    static void Shutdown();
    static void Render(ImGuiID dockspace_id);
    static void Clear();
};

} // namespace FDE
