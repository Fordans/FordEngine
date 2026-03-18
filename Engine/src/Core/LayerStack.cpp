#include "FDE/pch.hpp"
#include "FDE/Core/LayerStack.hpp"
#include <algorithm>

namespace FDE
{

LayerStack::~LayerStack()
{
    for (auto& layer : m_layers)
    {
        layer->OnDetach();
    }
}

void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
{
    m_layers.emplace(m_layers.begin() + m_layerInsertIndex, std::move(layer));
    m_layers[m_layerInsertIndex]->OnAttach();
    ++m_layerInsertIndex;
}

void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
{
    m_layers.push_back(std::move(overlay));
    m_layers.back()->OnAttach();
}

void LayerStack::PopLayer(Layer* layer)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [layer](const std::unique_ptr<Layer>& l) { return l.get() == layer; });
    if (it != m_layers.end() && it < m_layers.begin() + static_cast<std::ptrdiff_t>(m_layerInsertIndex))
    {
        (*it)->OnDetach();
        m_layers.erase(it);
        --m_layerInsertIndex;
    }
}

void LayerStack::PopOverlay(Layer* layer)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [layer](const std::unique_ptr<Layer>& l) { return l.get() == layer; });
    if (it != m_layers.end() && it >= m_layers.begin() + static_cast<std::ptrdiff_t>(m_layerInsertIndex))
    {
        (*it)->OnDetach();
        m_layers.erase(it);
    }
}

} // namespace FDE
