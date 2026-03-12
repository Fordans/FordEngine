#pragma once

#include "FDE/FDE.hpp"

class EditorApplication : public FDE::Application
{
  public:
    EditorApplication();
    ~EditorApplication();

  protected:
    FDE::WindowSpec GetWindowSpec() const override;

  private:
    bool Initialize();
};
