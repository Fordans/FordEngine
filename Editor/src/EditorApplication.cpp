#include "EditorApplication.hpp"

EditorApplication::EditorApplication()
{
    if (!Initialize())
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize EditorApplication");
    }
}

EditorApplication::~EditorApplication() {}

FDE::WindowSpec EditorApplication::GetWindowSpec() const
{
    FDE::WindowSpec spec;
    spec.width = 1600;
    spec.height = 900;
    spec.title = "Ford Editor";
    return spec;
}

bool EditorApplication::Initialize()
{
    // TODO: Initialize editor application here
    return true;
}