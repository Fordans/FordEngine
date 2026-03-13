#pragma once

#include "FDE/Export.hpp"

struct GLFWwindow;
struct ImFont;

namespace FDE
{

class FDE_API ImGuiContext
{
  public:
    ImGuiContext() = default;
    ~ImGuiContext();

    ImGuiContext(const ImGuiContext&) = delete;
    ImGuiContext& operator=(const ImGuiContext&) = delete;

    bool Init(GLFWwindow* window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    bool IsInitialized() const { return m_initialized; }

    ImFont* GetTitleFont() const { return m_titleFont; }

  private:
    bool m_initialized = false;
    ImFont* m_titleFont = nullptr;
};

} // namespace FDE
