#pragma once

#include "FDE/Export.hpp"
#include <memory>
#include <string>

struct GLFWwindow;

namespace FDE
{

struct WindowSpec
{
    int width = 1280;
    int height = 720;
    std::string title = "Ford Engine";
    bool decorated = true; // false = frameless, for custom title bar
    std::string iconPath;  // optional; path to PNG for window icon (taskbar, title bar)
};

class FDE_API Window
{
  public:
    explicit Window(const WindowSpec& spec);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void OnUpdate();
    void SwapBuffers();
    void RequestClose();
    void Minimize();
    void Maximize();
    void Restore();
    bool IsMaximized() const;
    void ProcessTitleBarDrag(float titleBarHeight, float excludeRightWidth = 0.0f);
    bool ShouldClose() const;
    bool IsValid() const { return m_window != nullptr; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    void GetFramebufferSize(int& width, int& height) const;
    const std::string& GetTitle() const { return m_title; }

    void* GetNativeWindow() const;
    GLFWwindow* GetGLFWWindow() const;

  private:
    struct GLFWwindow* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    std::string m_title;
};

} // namespace FDE
