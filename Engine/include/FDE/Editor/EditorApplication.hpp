#pragma once

#include "FDE/FDE.hpp"
#include "FDE/Project/ProjectDescriptor.hpp"
#include "FDE/Renderer/Camera2D.hpp"
#include "FDE/Renderer/Camera3D.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Renderer/Viewport.hpp"
#include "FDE/Scene/Object.hpp"
#include "FDE/Scene/Scene2D.hpp"
#include "FDE/Scene/Scene3D.hpp"
#include "FDE/Scene/World.hpp"
#include "FDE/Editor/EditorScene3DTools.hpp"
#include "imgui.h"
#include <memory>
#include <optional>

namespace FDE
{

class EditorPreferences;
class AssetManager;

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
    void OnBeforeImGuiNewFrame() override;
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
    void RenderSceneTreeView(ImGuiID dockspace_id);
    void RenderDetailView(ImGuiID dockspace_id);
    void RenderContentView(ImGuiID dockspace_id);
    void RenderSceneGridOverlay(ImVec2 itemMin, ImVec2 itemMax, uint32_t viewportWidth,
                               uint32_t viewportHeight);
    /// Apply 2D/3D scene camera input before rendering the viewport (uses prior-frame image rect for hover).
    void ProcessSceneViewportCameraInput(bool allowSceneCameraInput, uint32_t width, uint32_t height);
    void ReleaseSceneViewMouseCapture();
    void LoadContentViewIcons();
    void SyncEditorActiveScenes();

    void OnNewProject();
    void OnOpenProject();
    void OnSaveProject();
    void OnPlayInRuntime();
    void OnRegisterFileAssociation();
    void RefreshAssetPipeline();
    void OnRescanAssets();
    void OnBuildAssetPack();

  private:
    std::unique_ptr<EditorPreferences> m_preferences;
    bool m_showPreferences = false;
    bool m_showScene = true;
    bool m_showSceneTree = true;
    bool m_showDetail = true;
    bool m_showConsole = true;
    bool m_showContentView = true;
    void* m_titleBarIconTexture = nullptr;  // ImTextureID / GLuint
    float m_titleBarIconWidth = 0;
    float m_titleBarIconHeight = 0;
    void* m_contentViewFolderIcon = nullptr;
    void* m_contentViewFileIcon = nullptr;
    std::string m_contentViewCurrentPath;  // relative to project root
    std::unique_ptr<Viewport> m_sceneViewport;
    std::unique_ptr<World> m_world;
    Scene2D* m_scene2D = nullptr;
    Scene3D* m_scene3D = nullptr;
    Object m_selectedObject;  // Selected entity in Scene Tree (for future Inspector)
    Camera2D m_sceneCamera;
    Camera3D m_sceneCamera3D;
    ImVec2 m_sceneViewLastImageMin{0.0f, 0.0f};
    ImVec2 m_sceneViewLastImageMax{0.0f, 0.0f};
    uint32_t m_sceneViewLastWidth = 0;
    uint32_t m_sceneViewLastHeight = 0;
    bool m_sceneViewLayoutCached = false;
    /// RMB scene navigation: hide cursor, clip to scene image (Win32) or cursor disabled (other platforms).
    bool m_sceneViewMouseCaptured = false;
    Scene3DGizmoState m_scene3DGizmoState;
    std::optional<ProjectDescriptor> m_projectDescriptor;
    std::unique_ptr<AssetManager> m_assetManager;
};

} // namespace FDE
