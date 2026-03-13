#pragma once

#include "FDE/Export.hpp"

struct GLFWwindow;

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

  private:
    bool m_initialized = false;
};

} // namespace FDE
