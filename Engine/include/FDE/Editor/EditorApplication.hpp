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
    static constexpr float TITLE_BAR_HEIGHT = 28.0f;

  protected:
    WindowSpec GetWindowSpec() const override;
    void OnUpdate() override;

  private:
    bool Initialize();
    void RenderMainUI();
    void RenderTitleBar();
    void RenderPreferencesWindow();

  private:
    std::unique_ptr<EditorPreferences> m_preferences;
    bool m_showPreferences = false;
};

} // namespace FDE
