#include "FDE/pch.hpp"
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
constexpr const char* FILTER_PROJECT = "Project";
constexpr const char* KEY_LAST_PROJECT_PATH = "LastProjectPath";
constexpr const char* FILTER_CONTENT = "Content";
constexpr const char* KEY_CONTENT_ICON_SIZE = "IconSize";
constexpr int DEFAULT_CONTENT_ICON_SIZE = 48;
constexpr const char* FILTER_VIEW = "View";
constexpr const char* KEY_SHOW_SCENE = "ShowScene";
constexpr const char* KEY_SHOW_CONTENT = "ShowContent";
constexpr const char* KEY_SHOW_CONSOLE = "ShowConsole";
constexpr const char* KEY_SHOW_PREFERENCES = "ShowPreferences";
constexpr const char* KEY_SHOW_SCENE_TREE = "ShowSceneTree";
constexpr const char* KEY_SHOW_DETAIL = "ShowDetail";
constexpr const char* FILTER_SCENE = "Scene";
constexpr const char* KEY_SHOW_GRID = "ShowGrid";
constexpr const char* KEY_GRID_SIZE = "GridSize";
constexpr float DEFAULT_GRID_SIZE = 1.0f;
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

std::string EditorPreferences::GetLastProjectPath() const
{
    try
    {
        return m_impl->config.getConfig<std::string>(FILTER_PROJECT, KEY_LAST_PROJECT_PATH);
    }
    catch (const std::exception&)
    {
        return {};
    }
}

void EditorPreferences::SetLastProjectPath(const std::string& path)
{
    m_impl->config.setConfig(FILTER_PROJECT, KEY_LAST_PROJECT_PATH, path);
}

int EditorPreferences::GetContentIconSize() const
{
    try
    {
        return m_impl->config.getConfig<int>(FILTER_CONTENT, KEY_CONTENT_ICON_SIZE);
    }
    catch (const std::exception&)
    {
        return DEFAULT_CONTENT_ICON_SIZE;
    }
}

void EditorPreferences::SetContentIconSize(int size)
{
    m_impl->config.setConfig(FILTER_CONTENT, KEY_CONTENT_ICON_SIZE, size);
}

bool EditorPreferences::GetShowScene() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_VIEW, KEY_SHOW_SCENE);
    }
    catch (const std::exception&)
    {
        return true;
    }
}

void EditorPreferences::SetShowScene(bool show)
{
    m_impl->config.setConfig(FILTER_VIEW, KEY_SHOW_SCENE, show);
}

bool EditorPreferences::GetShowContent() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_VIEW, KEY_SHOW_CONTENT);
    }
    catch (const std::exception&)
    {
        return true;
    }
}

void EditorPreferences::SetShowContent(bool show)
{
    m_impl->config.setConfig(FILTER_VIEW, KEY_SHOW_CONTENT, show);
}

bool EditorPreferences::GetShowConsole() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_VIEW, KEY_SHOW_CONSOLE);
    }
    catch (const std::exception&)
    {
        return true;
    }
}

void EditorPreferences::SetShowConsole(bool show)
{
    m_impl->config.setConfig(FILTER_VIEW, KEY_SHOW_CONSOLE, show);
}

bool EditorPreferences::GetShowPreferences() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_VIEW, KEY_SHOW_PREFERENCES);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void EditorPreferences::SetShowPreferences(bool show)
{
    m_impl->config.setConfig(FILTER_VIEW, KEY_SHOW_PREFERENCES, show);
}

bool EditorPreferences::GetShowSceneTree() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_VIEW, KEY_SHOW_SCENE_TREE);
    }
    catch (const std::exception&)
    {
        return true;
    }
}

void EditorPreferences::SetShowSceneTree(bool show)
{
    m_impl->config.setConfig(FILTER_VIEW, KEY_SHOW_SCENE_TREE, show);
}

bool EditorPreferences::GetShowDetail() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_VIEW, KEY_SHOW_DETAIL);
    }
    catch (const std::exception&)
    {
        return true;
    }
}

void EditorPreferences::SetShowDetail(bool show)
{
    m_impl->config.setConfig(FILTER_VIEW, KEY_SHOW_DETAIL, show);
}

bool EditorPreferences::GetShowSceneGrid() const
{
    try
    {
        return m_impl->config.getConfig<bool>(FILTER_SCENE, KEY_SHOW_GRID);
    }
    catch (const std::exception&)
    {
        return true;
    }
}

void EditorPreferences::SetShowSceneGrid(bool show)
{
    m_impl->config.setConfig(FILTER_SCENE, KEY_SHOW_GRID, show);
}

float EditorPreferences::GetSceneGridSize() const
{
    try
    {
        return m_impl->config.getConfig<float>(FILTER_SCENE, KEY_GRID_SIZE);
    }
    catch (const std::exception&)
    {
        return DEFAULT_GRID_SIZE;
    }
}

void EditorPreferences::SetSceneGridSize(float size)
{
    m_impl->config.setConfig(FILTER_SCENE, KEY_GRID_SIZE, size);
}

} // namespace FDE
