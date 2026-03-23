#pragma once

#include "FDE/Export.hpp"
#include <memory>
#include <string>

namespace FDE
{

class FDE_API EditorPreferences
{
  public:
    EditorPreferences();
    ~EditorPreferences();

    EditorPreferences(const EditorPreferences&) = delete;
    EditorPreferences& operator=(const EditorPreferences&) = delete;

    // Window
    int GetWindowWidth() const;
    int GetWindowHeight() const;
    void SetWindowSize(int width, int height);
    bool GetMaximized() const;
    void SetMaximized(bool maximized);

    const std::string& GetConfigPath() const { return m_configPath; }

    // Last opened project path (for "Open Recent")
    std::string GetLastProjectPath() const;
    void SetLastProjectPath(const std::string& path);

    /// false = show start wizard on launch (default); true = try to open last project folder.
    bool GetStartupLoadLastProject() const;
    void SetStartupLoadLastProject(bool loadLast);

    // Content View
    int GetContentIconSize() const;
    void SetContentIconSize(int size);

    // View panel visibility
    bool GetShowScene() const;
    void SetShowScene(bool show);
    bool GetShowContent() const;
    void SetShowContent(bool show);
    bool GetShowConsole() const;
    void SetShowConsole(bool show);
    bool GetShowPreferences() const;
    void SetShowPreferences(bool show);
    bool GetShowSceneTree() const;
    void SetShowSceneTree(bool show);
    bool GetShowDetail() const;
    void SetShowDetail(bool show);

    // Scene grid (editor view)
    bool GetShowSceneGrid() const;
    void SetShowSceneGrid(bool show);
    float GetSceneGridSize() const;
    void SetSceneGridSize(float size);

    /// 3D Scene view: scales mouse look, WASD/QE move, and scroll dolly (1.0 = default).
    float GetScene3DNavSensitivity() const;
    void SetScene3DNavSensitivity(float value);

    /// 0 = Position, 1 = Rotate, 2 = Scale (3D Scene transform gizmo).
    int GetScene3DTransformMode() const;
    void SetScene3DTransformMode(int mode);

  private:
    std::string m_configPath;
    struct ConfigImpl;
    std::unique_ptr<ConfigImpl> m_impl;
};

} // namespace FDE
