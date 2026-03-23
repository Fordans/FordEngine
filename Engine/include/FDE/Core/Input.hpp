#pragma once

#include "FDE/Export.hpp"

namespace FDE
{

class Window;

/// GLFW keyboard state for the current frame (game code; ImGui capture handled by caller).
class FDE_API Input
{
  public:
    static void SetCurrentWindow(Window* window);
    static Window* GetCurrentWindow();

    static bool IsKeyDown(int glfwKey);

  private:
    static Window* s_window;
};

} // namespace FDE
