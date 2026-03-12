#pragma once

#include "FDE/FDE.hpp"

class EditorApplication : public FDE::Application
{
  public:
    EditorApplication();
    ~EditorApplication();

  private:
    bool Initialize();
};
