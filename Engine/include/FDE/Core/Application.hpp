#pragma once

#include "FDE/Export.hpp"

namespace FDE
{

class FDE_API Application
{
  public:
    Application() = default;
    virtual ~Application() = default;

    virtual void Run();

  private:
    bool m_isRunning = true;
};

/*
 Implemented by client
 Customize your own Application
 Reference: EditorApplication
*/
Application* CreateApplication();

} // namespace FDE
