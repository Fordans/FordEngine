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

  private:
    std::string m_configPath;
    struct ConfigImpl;
    std::unique_ptr<ConfigImpl> m_impl;
};

} // namespace FDE
