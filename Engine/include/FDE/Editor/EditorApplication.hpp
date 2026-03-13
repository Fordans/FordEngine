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
    static constexpr float TITLE_BAR_HEIGHT = 64.0f;
    static constexpr float TITLE_BAR_BUTTON_WIDTH = 64.0f;
    static constexpr float TITLE_BAR_BUTTON_AREA_WIDTH = 218.0f;  // 3 * button + spacing

  protected:
    WindowSpec GetWindowSpec() const override;
    void OnUpdate() override;

  private:
    bool Initialize();
    void LoadTitleBarIcon();
    void RenderMainUI();
    void RenderTitleBar();
    void RenderPreferencesWindow();

  private:
    std::unique_ptr<EditorPreferences> m_preferences;
    bool m_showPreferences = false;
    void* m_titleBarIconTexture = nullptr;  // ImTextureID / GLuint
    float m_titleBarIconWidth = 0;
    float m_titleBarIconHeight = 0;
};

} // namespace FDE
