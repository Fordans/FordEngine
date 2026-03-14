#pragma once

#include "FDE/Core/Layer.hpp"
#include "FDE/Export.hpp"
#include <memory>
#include <vector>

namespace FDE
{

class FDE_API LayerStack
{
  public:
    LayerStack() = default;
    ~LayerStack();

    LayerStack(const LayerStack&) = delete;
    LayerStack& operator=(const LayerStack&) = delete;

    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* layer);

    std::vector<std::unique_ptr<Layer>>::iterator begin() { return m_layers.begin(); }
    std::vector<std::unique_ptr<Layer>>::iterator end() { return m_layers.end(); }
    std::vector<std::unique_ptr<Layer>>::reverse_iterator rbegin() { return m_layers.rbegin(); }
    std::vector<std::unique_ptr<Layer>>::reverse_iterator rend() { return m_layers.rend(); }

  private:
    std::vector<std::unique_ptr<Layer>> m_layers;
    unsigned int m_layerInsertIndex = 0;
};

} // namespace FDE
