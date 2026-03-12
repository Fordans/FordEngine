#include "FDE/Window/Window.hpp"
#include "FDE/Core/Log.hpp"
#include <GLFW/glfw3.h>

namespace FDE
{

Window::Window(const WindowSpec& spec)
    : m_width(spec.width)
    , m_height(spec.height)
    , m_title(spec.title)
{
    if (!glfwInit())
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize GLFW");
        return;
    }

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        FDE_LOG_CLIENT_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);
}

Window::~Window()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void Window::OnUpdate()
{
    glfwPollEvents();
}

bool Window::ShouldClose() const
{
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void* Window::GetNativeWindow() const
{
    return static_cast<void*>(m_window);
}

} // namespace FDE
