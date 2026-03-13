#include "FDE/Window/Window.hpp"
#include "FDE/Core/Log.hpp"
#include <GLFW/glfw3.h>

namespace FDE
{

static void GlfwErrorCallback(int error, const char* description)
{
    FDE_LOG_CLIENT_ERROR("GLFW Error {}: {}", error, description);
}

Window::Window(const WindowSpec& spec)
    : m_width(spec.width)
    , m_height(spec.height)
    , m_title(spec.title)
{
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit())
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize GLFW");
        return;
    }

    // OpenGL 3.0 context for ImGui
#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        FDE_LOG_CLIENT_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync
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

void Window::SwapBuffers()
{
    if (m_window)
    {
        glfwSwapBuffers(m_window);
    }
}

void Window::RequestClose()
{
    if (m_window)
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

void Window::GetFramebufferSize(int& width, int& height) const
{
    if (m_window)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
    }
    else
    {
        width = 0;
        height = 0;
    }
}

bool Window::ShouldClose() const
{
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void* Window::GetNativeWindow() const
{
    return static_cast<void*>(m_window);
}

GLFWwindow* Window::GetGLFWWindow() const
{
    return m_window;
}

} // namespace FDE
