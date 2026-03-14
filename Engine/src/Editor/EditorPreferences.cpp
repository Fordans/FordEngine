#include "FDE/Editor/EditorPreferences.hpp"
#include "FDS_ConfigManager.h"

namespace FDE
{

struct EditorPreferences::ConfigImpl
{
    FDS_ConfigManager config;
    ConfigImpl(const std::string& path) : config(path) {}
};

namespace
{
constexpr int DEFAULT_WIDTH = 1920;
constexpr int DEFAULT_HEIGHT = 1080;
constexpr const char* FILTER_WINDOW = "Window";
constexpr const char* KEY_WIDTH = "Width";
constexpr const char* KEY_HEIGHT = "Height";
constexpr const char* KEY_MAXIMIZED = "Maximized";
} // namespace

EditorPreferences::EditorPreferences() : m_configPath("FordEditor.cfg"), m_impl(std::make_unique<ConfigImpl>(m_configPath))
{
}

EditorPreferences::~EditorPreferences() = default;

int EditorPreferences::GetWindowWidth() const
{
    try
    {
        return m_impl->config.getConfig<int>(FILTER_WINDOW, KEY_WIDTH);
    }
    catch (const std::exception&)
    {
        return DEFAULT_WIDTH;
    }
}

int EditorPreferences::GetWindowHeight() const
{
    try
    {
        return m_impl->config.getConfig<int>(FILTER_WINDOW, KEY_HEIGHT);
    }
    catch (const std::exception&)
    {
        return DEFAULT_HEIGHT;
    }
}

void EditorPreferences::SetWindowSize(int width, int height)
{
    m_impl->config.setConfig(FILTER_WINDOW, KEY_WIDTH, width);
    m_impl->config.setConfig(FILTER_WINDOW, KEY_HEIGHT, height);
}

bool EditorPreferences::GetMaximized() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_WINDOW, KEY_MAXIMIZED);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void EditorPreferences::SetMaximized(bool maximized)
{
    m_impl->config.setConfig(FILTER_WINDOW, KEY_MAXIMIZED, maximized);
}

} // namespace FDE
