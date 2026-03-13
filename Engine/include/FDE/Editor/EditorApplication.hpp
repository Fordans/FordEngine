#pragma once

#include "FDE/FDE.hpp"
#include <memory>

namespace FDE
{

class EditorPreferences;

class FDE_API EditorApplication : public Application
{
  public:
    EditorApplication();
    ~EditorApplication();

  protected:
    WindowSpec GetWindowSpec() const override;
    void OnUpdate() override;

  private:
    bool Initialize();
    void RenderMainUI();
    void RenderPreferencesWindow();

  private:
    std::unique_ptr<EditorPreferences> m_preferences;
    bool m_showPreferences = false;
};

} // namespace FDE
