#pragma once

#include "FDE/FDE.hpp"

namespace FDE
{

class FDE_API EditorApplication : public Application
{
  public:
    EditorApplication();
    ~EditorApplication();

  protected:
    WindowSpec GetWindowSpec() const override;
    void OnUpdate() override;

  private:
    bool Initialize();
    void RenderMainUI();
};

} // namespace FDE
