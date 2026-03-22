#pragma once

#include "FDE/Export.hpp"
#include "FDE/ImGui/ImGuiContext.hpp"
#include "FDE/Window/Window.hpp"
#include "FDE/Core/LayerStack.hpp"
#include "FDE/Core/Event.hpp"
#include <memory>

namespace FDE
{

class FDE_API Application
{
  public:
    Application() = default;
    virtual ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    virtual void Run();
    /// Called after glfwPollEvents, before ImGui NewFrame (e.g. sync io.ConfigFlags with capture state).
    virtual void OnBeforeImGuiNewFrame() {}
    virtual void OnUpdate() {}
    virtual void OnWindowCreated() {}
    virtual void OnRunEnd() {}

    Window* GetWindow() { return m_window.get(); }
    const Window* GetWindow() const { return m_window.get(); }
    ImGuiContext* GetImGuiContext() { return m_imgui.get(); }
    const ImGuiContext* GetImGuiContext() const { return m_imgui.get(); }
    LayerStack& GetLayerStack() { return m_layerStack; }
    const LayerStack& GetLayerStack() const { return m_layerStack; }

    void DispatchEvent(Event& event);

  protected:
    virtual WindowSpec GetWindowSpec() const { return {}; }

  private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<ImGuiContext> m_imgui;
    LayerStack m_layerStack;
};

} // namespace FDE
