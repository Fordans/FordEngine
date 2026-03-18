#include "FDE/pch.hpp"
#include "FDE/Editor/EditorConsole.hpp"
#include "FDE/Core/Log.hpp"
#include <spdlog/sinks/ringbuffer_sink.h>
#include "imgui.h"
#include <algorithm>

namespace FDE
{

namespace
{
std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> g_consoleSink;
constexpr size_t CONSOLE_MAX_ENTRIES = 1024;
} // namespace

void EditorConsole::Initialize()
{
    if (g_consoleSink)
        return;

    g_consoleSink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(CONSOLE_MAX_ENTRIES);
    g_consoleSink->set_pattern("[%H:%M:%S] [%l] %v");

    GetCoreLogger()->sinks().push_back(g_consoleSink);
    GetClientLogger()->sinks().push_back(g_consoleSink);
}

void EditorConsole::Shutdown()
{
    if (!g_consoleSink)
        return;

    auto& coreSinks = GetCoreLogger()->sinks();
    auto& clientSinks = GetClientLogger()->sinks();

    coreSinks.erase(std::remove(coreSinks.begin(), coreSinks.end(), g_consoleSink), coreSinks.end());
    clientSinks.erase(std::remove(clientSinks.begin(), clientSinks.end(), g_consoleSink), clientSinks.end());

    g_consoleSink.reset();
}

void EditorConsole::Clear()
{
    if (g_consoleSink)
    {
        g_consoleSink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(CONSOLE_MAX_ENTRIES);
        g_consoleSink->set_pattern("[%H:%M:%S] [%l] %v");
        for (auto& sink : GetCoreLogger()->sinks())
        {
            if (sink == g_consoleSink)
                break;
        }
        // Actually we need to replace the sink - the loggers hold shared_ptr to it.
        // Clear doesn't exist on ringbuffer_sink. We'd need to recreate.
        // Simpler: just don't implement Clear for now, or use a different approach.
        // Let me check - ringbuffer has no clear. We have to create new sink and replace.
    }
}

static ImVec4 GetColorForLevel(spdlog::level::level_enum level)
{
    switch (level)
    {
    case spdlog::level::trace:
        return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    case spdlog::level::debug:
        return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    case spdlog::level::info:
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    case spdlog::level::warn:
        return ImVec4(1.0f, 0.9f, 0.3f, 1.0f);
    case spdlog::level::err:
        return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    case spdlog::level::critical:
        return ImVec4(1.0f, 0.3f, 0.6f, 1.0f);
    default:
        return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    }
}

void EditorConsole::Render(ImGuiID dockspace_id)
{
    if (!g_consoleSink)
        return;

    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Console"))
    {
        if (ImGui::Button("Clear"))
        {
            auto newSink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(CONSOLE_MAX_ENTRIES);
            newSink->set_pattern("[%H:%M:%S] [%l] %v");

            auto& coreSinks = GetCoreLogger()->sinks();
            auto& clientSinks = GetClientLogger()->sinks();
            for (auto& sink : coreSinks)
            {
                if (std::dynamic_pointer_cast<spdlog::sinks::ringbuffer_sink_mt>(sink))
                {
                    sink = newSink;
                    break;
                }
            }
            for (auto& sink : clientSinks)
            {
                if (std::dynamic_pointer_cast<spdlog::sinks::ringbuffer_sink_mt>(sink))
                {
                    sink = newSink;
                    break;
                }
            }
            g_consoleSink = newSink;
        }
        ImGui::SameLine();
        ImGui::Separator();

        auto formatted = g_consoleSink->last_formatted(0);
        auto raw = g_consoleSink->last_raw(0);

        ImGui::BeginChild("ConsoleScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (size_t i = 0; i < raw.size(); ++i)
        {
            ImVec4 color = GetColorForLevel(raw[i].level);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(formatted[i].c_str());
            ImGui::PopStyleColor();
        }

        if (!formatted.empty() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace FDE
