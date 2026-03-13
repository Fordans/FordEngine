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

    const std::string& GetConfigPath() const { return m_configPath; }

  private:
    std::string m_configPath;
    struct ConfigImpl;
    std::unique_ptr<ConfigImpl> m_impl;
};

} // namespace FDE
