#pragma once

#include "FDE/Core/Layer.hpp"
#include "FDE/Export.hpp"

namespace FDE
{

class RuntimeSession;

/// Optional ImGui overlay for standalone runtime (stats, entity list). Toggle with F3 from `RuntimeApplication`.
class FDE_API RuntimeDebugLayer : public Layer
{
  public:
    RuntimeDebugLayer(RuntimeSession* session, float* smoothedFps, bool* debugUiEnabled);

    void OnUpdate() override;

  private:
    RuntimeSession* m_session = nullptr;
    float* m_smoothedFps = nullptr;
    bool* m_debugUiEnabled = nullptr;
};

} // namespace FDE
