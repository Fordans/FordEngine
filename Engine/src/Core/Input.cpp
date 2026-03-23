#include "FDE/pch.hpp"
#include "FDE/Core/Input.hpp"
#include "FDE/Window/Window.hpp"
#include <GLFW/glfw3.h>

namespace FDE
{

Window* Input::s_window = nullptr;

void Input::SetCurrentWindow(Window* window)
{
    s_window = window;
}

Window* Input::GetCurrentWindow()
{
    return s_window;
}

bool Input::IsKeyDown(int glfwKey)
{
    if (!s_window)
        return false;
    GLFWwindow* w = s_window->GetGLFWWindow();
    if (!w)
        return false;
    return glfwGetKey(w, glfwKey) == GLFW_PRESS;
}

} // namespace FDE
