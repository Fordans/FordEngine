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
};

class FDE_API Window
{
  public:
    explicit Window(const WindowSpec& spec);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void OnUpdate();
    bool ShouldClose() const;
    bool IsValid() const { return m_window != nullptr; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    const std::string& GetTitle() const { return m_title; }

    void* GetNativeWindow() const;

  private:
    struct GLFWwindow* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    std::string m_title;
};

} // namespace FDE
