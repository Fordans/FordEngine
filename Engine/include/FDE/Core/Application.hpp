#pragma once

#include "FDE/Export.hpp"
#include "FDE/Window/Window.hpp"
#include <memory>

namespace FDE
{

class FDE_API Application
{
  public:
    Application() = default;
    virtual ~Application() = default;

    virtual void Run();
    virtual void OnUpdate() {}

    Window* GetWindow() { return m_window.get(); }
    const Window* GetWindow() const { return m_window.get(); }

  protected:
    virtual WindowSpec GetWindowSpec() const { return {}; }

  private:
    std::unique_ptr<Window> m_window;
};

/*
 Implemented by client
 Customize your own Application
 Reference: EditorApplication
*/
Application* CreateApplication();

} // namespace FDE
