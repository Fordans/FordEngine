#pragma once

#include "FDE/Core/Layer.hpp"

namespace FDE
{

class FDE_API ImGuiLayer : public Layer
{
  public:
    ImGuiLayer();
    ~ImGuiLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
    void OnEvent(Event& event) override;
};

} // namespace FDE
