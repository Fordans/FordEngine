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

    glfwWindowHint(GLFW_DECORATED, spec.decorated ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        FDE_LOG_CLIENT_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    // 无边框窗口居中显示，避免贴边导致标题栏被遮挡无法拖动
    if (!spec.decorated)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor)
        {
            int workX, workY, workW, workH;
            glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);
            int posX = workX + (workW - m_width) / 2;
            int posY = workY + (workH - m_height) / 2;
            glfwSetWindowPos(m_window, posX, posY);
        }
    }
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

void Window::Minimize()
{
    if (m_window)
        glfwIconifyWindow(m_window);
}

void Window::Maximize()
{
    if (m_window)
        glfwMaximizeWindow(m_window);
}

void Window::Restore()
{
    if (m_window)
        glfwRestoreWindow(m_window);
}

bool Window::IsMaximized() const
{
    return m_window && glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED);
}

void Window::ProcessTitleBarDrag(float titleBarHeight, float excludeRightWidth)
{
    if (!m_window || titleBarHeight <= 0.0f)
        return;

    static bool s_dragging = false;
    static double s_prevScreenX = 0, s_prevScreenY = 0;

    int winW, winH;
    glfwGetWindowSize(m_window, &winW, &winH);

    double cursorX, cursorY;
    glfwGetCursorPos(m_window, &cursorX, &cursorY);
    int winX, winY;
    glfwGetWindowPos(m_window, &winX, &winY);
    double screenX = winX + cursorX;
    double screenY = winY + cursorY;

    int mouseButton = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT);

    if (mouseButton == GLFW_PRESS)
    {
        bool inTitleArea = cursorY < static_cast<double>(titleBarHeight);
        bool inButtonArea = excludeRightWidth > 0 && cursorX >= winW - static_cast<double>(excludeRightWidth);
        if (!s_dragging && inTitleArea && !inButtonArea)
        {
            s_dragging = true;
            s_prevScreenX = screenX;
            s_prevScreenY = screenY;
        }
        if (s_dragging)
        {
            glfwSetWindowPos(m_window, winX + static_cast<int>(screenX - s_prevScreenX),
                            winY + static_cast<int>(screenY - s_prevScreenY));
            s_prevScreenX = screenX;
            s_prevScreenY = screenY;
        }
    }
    else
    {
        s_dragging = false;
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
