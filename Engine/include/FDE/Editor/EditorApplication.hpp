#pragma once

#include "FDE/FDE.hpp"
#include "FDE/Project/ProjectDescriptor.hpp"
#include "FDE/Renderer/Camera2D.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Renderer/Viewport.hpp"
#include "imgui.h"
#include <memory>
#include <optional>

namespace FDE
{

class EditorPreferences;

class FDE_API EditorApplication : public Application
{
  public:
    /// \param initialProjectPath Optional .fproject file path (e.g. from double-click in Explorer).
    explicit EditorApplication(const std::string& initialProjectPath = {});
    ~EditorApplication();
    static constexpr float TITLE_BAR_HEIGHT = 64.0f;
    static constexpr float TITLE_BAR_BUTTON_WIDTH = 64.0f;
    static constexpr float TITLE_BAR_BUTTON_AREA_WIDTH = 218.0f;  // 3 * button + spacing

  protected:
    WindowSpec GetWindowSpec() const override;
    void OnUpdate() override;
    void OnWindowCreated() override;
    void OnRunEnd() override;

  private:
    bool Initialize();
    void LoadTitleBarIcon();
    void RenderMainUI();
    void RenderTitleBar();
    void RenderPreferencesWindow(ImGuiID dockspace_id);
    void RenderSceneView(ImGuiID dockspace_id);
    void RenderContentView(ImGuiID dockspace_id);
    void LoadContentViewIcons();

    void OnNewProject();
    void OnOpenProject();
    void OnSaveProject();
    void OnRegisterFileAssociation();

  private:
    std::unique_ptr<EditorPreferences> m_preferences;
    bool m_showPreferences = false;
    bool m_showScene = true;
    bool m_showConsole = true;
    bool m_showContentView = true;
    void* m_titleBarIconTexture = nullptr;  // ImTextureID / GLuint
    float m_titleBarIconWidth = 0;
    float m_titleBarIconHeight = 0;
    void* m_contentViewFolderIcon = nullptr;
    void* m_contentViewFileIcon = nullptr;
    std::string m_contentViewCurrentPath;  // relative to project root
    std::unique_ptr<Viewport> m_sceneViewport;
    std::shared_ptr<VertexArray> m_triangleVAO;
    Camera2D m_sceneCamera;
    std::optional<ProjectDescriptor> m_projectDescriptor;
};

} // namespace FDE
