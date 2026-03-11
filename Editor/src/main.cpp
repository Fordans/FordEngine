#include "EditorApplication.hpp"

FDE::Application* FDE::CreateApplication()
{
    return new EditorApplication();
}

int main(int argc, char* argv[])
{
    auto app = FDE::CreateApplication();
    app->Run();

    delete app;
    return 0;
}
