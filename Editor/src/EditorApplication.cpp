#include "EditorApplication.hpp"

EditorApplication::EditorApplication()
{
    if (!Initialize())
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize EditorApplication");
    }
}

EditorApplication::~EditorApplication() {}

bool EditorApplication::Initialize()
{
    // TODO: Initialize editor application here
    return true;
}