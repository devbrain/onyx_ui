// widget::show_auto_tooltip() implementation — included after label.hh is available

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/tooltip_widget.hh>

namespace onyxui {

    template<UIBackend Backend>
    void widget<Backend>::show_auto_tooltip() {
        if (m_tooltip_text.empty()) return;

        auto* layers = ui_services<Backend>::layers();
        if (!layers) return;

        auto tip = std::make_unique<tooltip_widget<Backend>>(m_tooltip_text);

        auto abs = this->get_absolute_logical_bounds();
        auto id = layers->show_tooltip(
            tip.get(),
            abs.x,
            abs.y + abs.height);

        m_tooltip_content = std::move(tip);
        m_tooltip_layer = scoped_layer<Backend>(layers, id);
    }

} // namespace onyxui
