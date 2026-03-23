#pragma once

#include "FDE/Core/Application.hpp"
#include "FDE/Export.hpp"
#include "FDE/Runtime/RuntimeSession.hpp"
#include <string>

namespace FDE
{

/// Standalone player: loads a `.fproject`, runs the active 3D scene full-window, optional ImGui debug (F3).
///
/// Layer / input policy (see `RuntimeApplication.cpp`):
/// - Overlays (reverse event order): `ImGuiLayer` outermost, then `RuntimeDebugLayer`, so ImGui capture runs first.
/// - When debug UI is off, `OnBeforeImGuiNewFrame` sets `ImGuiConfigFlags_NoMouse` so gameplay keeps mouse input.
class FDE_API RuntimeApplication : public Application
{
  public:
    /// \param fprojectPath Absolute or cwd-relative path to `.fproject`.
    explicit RuntimeApplication(std::string fprojectPath);
    ~RuntimeApplication() override;

    RuntimeApplication(const RuntimeApplication&) = delete;
    RuntimeApplication& operator=(const RuntimeApplication&) = delete;

    RuntimeSession& GetSession() { return m_session; }
    const RuntimeSession& GetSession() const { return m_session; }

    bool IsDebugUIEnabled() const { return m_debugUIEnabled; }

  protected:
    WindowSpec GetWindowSpec() const override;
    void OnBeforeImGuiNewFrame() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnWindowCreated() override;
    void OnRunEnd() override;

  private:
    std::string m_fprojectPath;
    RuntimeSession m_session;
    bool m_loadOk = false;
    bool m_debugUIEnabled;
    bool m_f3WasDown = false;
    bool m_rmbDown = false;
    double m_lastFrameTime = 0.0;
    double m_lastCursorX = 0.0;
    double m_lastCursorY = 0.0;
    float m_smoothedFps = 0.0f;
};

} // namespace FDE
