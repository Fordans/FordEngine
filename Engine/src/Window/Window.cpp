#include "FDE/Window/Window.hpp"
#include "FDE/Core/Events/KeyEvent.hpp"
#include "FDE/Core/Events/MouseEvent.hpp"
#include "FDE/Core/Events/WindowEvent.hpp"
#include "FDE/Core/FileSystem.hpp"
#include "FDE/Core/Log.hpp"
#include <GLFW/glfw3.h>
#include "stb_image.h"

namespace FDE
{

static void GlfwErrorCallback(int error, const char* description)
{
    FDE_LOG_CLIENT_ERROR("GLFW Error {}: {}", error, description);
}

static void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!w)
        return;
    if (action == GLFW_PRESS)
    {
        KeyPressedEvent e(key, 0);
        w->DispatchEventToCallback(e);
    }
    else if (action == GLFW_RELEASE)
    {
        KeyReleasedEvent e(key);
        w->DispatchEventToCallback(e);
    }
}

static void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!w)
        return;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (action == GLFW_PRESS)
    {
        MouseButtonPressedEvent e(button, static_cast<float>(x), static_cast<float>(y));
        w->DispatchEventToCallback(e);
    }
    else if (action == GLFW_RELEASE)
    {
        MouseButtonReleasedEvent e(button, static_cast<float>(x), static_cast<float>(y));
        w->DispatchEventToCallback(e);
    }
}

static void GlfwCursorPosCallback(GLFWwindow* window, double x, double y)
{
    auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!w)
        return;
    MouseMovedEvent e(static_cast<float>(x), static_cast<float>(y));
    w->DispatchEventToCallback(e);
}

static void GlfwScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!w)
        return;
    MouseScrolledEvent e(static_cast<float>(xOffset), static_cast<float>(yOffset));
    w->DispatchEventToCallback(e);
}

static void GlfwWindowFocusCallback(GLFWwindow* window, int focused)
{
    auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!w)
        return;
    if (focused)
    {
        WindowFocusEvent e;
        w->DispatchEventToCallback(e);
    }
    else
    {
        WindowLostFocusEvent e;
        w->DispatchEventToCallback(e);
    }
}

static void GlfwWindowCloseCallback(GLFWwindow* window)
{
    auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!w)
        return;
    WindowCloseEvent e;
    w->DispatchEventToCallback(e);
}

static void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!w)
        return;
    WindowResizeEvent e(width, height);
    w->DispatchEventToCallback(e);
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

    // 设置窗口图标（任务栏、标题栏）
    std::string iconPath = spec.iconPath.empty() ? FileSystem::ResolveEngineResource("FE.png") : spec.iconPath;
    if (!iconPath.empty())
    {
        int iconW, iconH, channels;
        unsigned char* iconData = stbi_load(iconPath.c_str(), &iconW, &iconH, &channels, 4);
        if (iconData)
        {
            GLFWimage img = { iconW, iconH, iconData };
            glfwSetWindowIcon(m_window, 1, &img);
            stbi_image_free(iconData);
        }
    }

    glfwSetWindowUserPointer(m_window, this);
    InitCallbacks();
}

void Window::InitCallbacks()
{
    if (!m_window)
        return;
    glfwSetKeyCallback(m_window, GlfwKeyCallback);
    glfwSetMouseButtonCallback(m_window, GlfwMouseButtonCallback);
    glfwSetCursorPosCallback(m_window, GlfwCursorPosCallback);
    glfwSetScrollCallback(m_window, GlfwScrollCallback);
    glfwSetWindowFocusCallback(m_window, GlfwWindowFocusCallback);
    glfwSetWindowCloseCallback(m_window, GlfwWindowCloseCallback);
    glfwSetFramebufferSizeCallback(m_window, GlfwFramebufferSizeCallback);
}

void Window::DispatchEventToCallback(Event& e)
{
    if (m_eventCallback)
        m_eventCallback(e);
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
            int newX = winX + static_cast<int>(screenX - s_prevScreenX);
            int newY = winY + static_cast<int>(screenY - s_prevScreenY);

            // 限制在显示器工作区内，避免拖到任务栏下方无法找回
            GLFWmonitor* monitor = glfwGetWindowMonitor(m_window);
            if (!monitor)
            {
                int centerX = newX + winW / 2, centerY = newY + winH / 2;
                int count;
                GLFWmonitor** monitors = glfwGetMonitors(&count);
                for (int i = 0; i < count; ++i)
                {
                    int mx, my, mw, mh;
                    glfwGetMonitorWorkarea(monitors[i], &mx, &my, &mw, &mh);
                    if (centerX >= mx && centerX < mx + mw && centerY >= my && centerY < my + mh)
                    {
                        monitor = monitors[i];
                        break;
                    }
                }
                if (!monitor)
                    monitor = glfwGetPrimaryMonitor();
            }
            if (monitor)
            {
                int workX, workY, workW, workH;
                glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);
                // 仅限制标题栏不超出工作区上下，保证可拖回
                if (newY < workY)
                    newY = workY;
                if (newY + static_cast<int>(titleBarHeight) > workY + workH)
                    newY = workY + workH - static_cast<int>(titleBarHeight);
            }

            glfwSetWindowPos(m_window, newX, newY);
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
