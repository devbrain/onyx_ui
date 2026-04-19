/**
 * @file conio.hh
 * @brief Per-backend alias header for the conio backend.
 *
 * Mirrors `<onyxui/backend/sdlpp.hh>` under `namespace onyxui::conio`.
 * See that file's doc comment for the full mechanism.
 */

#pragma once

#include <onyxui/conio/conio_backend.hh>

#include <onyxui/core/element.hh>
#include <onyxui/ui_host.hh>

#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/icon.hh>
#include <onyxui/widgets/status_bar.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/progress_bar.hh>
#include <onyxui/widgets/main_window.hh>

#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/grid.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/tab_widget.hh>
#include <onyxui/widgets/containers/stack_panel.hh>
#include <onyxui/widgets/containers/anchor_panel.hh>
#include <onyxui/widgets/containers/absolute_panel.hh>
#include <onyxui/widgets/containers/scroll_view.hh>

#include <onyxui/widgets/containers/scroll/scrollable.hh>
#include <onyxui/widgets/containers/scroll/scrollbar.hh>
#include <onyxui/widgets/containers/scroll/scroll_controller.hh>

#include <onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/layout/spring.hh>

#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/slider.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/widgets/input/list_box.hh>
#include <onyxui/widgets/input/button_group.hh>

#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/widgets/menu/separator.hh>

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/dialog.hh>
#include <onyxui/widgets/window/presented_window.hh>

namespace onyxui::conio {

    /// Canonical name for the backend fixed by this shell header.
    using backend = conio_backend;

    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

} // namespace onyxui::conio
