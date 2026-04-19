/// @file test_backend_aliases.cc
/// @brief Smoke tests for the per-backend alias headers (WAR-52).
///
/// These tests don't exercise behavior — they exist to ensure that
/// the alias machinery (`include/onyxui/detail/public_types.inc`
/// + shell headers under `include/onyxui/backend/`) produces valid
/// aliases for every type on the canonical list.
///
/// Three layers of coverage:
///
/// 1. **Always-on** — exercise the X-macro mechanism against
///    `onyxui::testing::test_backend`, which is always available in
///    the unittest build. Proves `public_types.inc` itself is
///    correctly structured and every listed type fits the
///    single-backend-parameter template shape.
/// 2. **sdlpp integration** — include the real
///    `<onyxui/backend/sdlpp.hh>` when the sdlpp backend is
///    available in the build. Gated by `__has_include`.
/// 3. **conio integration** — same for `<onyxui/backend/conio.hh>`.

#include <doctest/doctest.h>

#include <memory>
#include <type_traits>

#include "utils/test_backend.hh"

// --------------------------------------------------------------------
// Layer 1: test_backend smoke — always built
// --------------------------------------------------------------------
//
// Replicate what <onyxui/backend/sdlpp.hh> does, but against the
// in-unittest test_backend. If `public_types.inc` ever references a
// non-single-backend-parameter template, this TU fails to compile.

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

namespace test_backend_aliases {

    using backend = ::onyxui::test_backend;

    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

    // sizeof() forces the compiler to instantiate the template just
    // enough to verify it exists and matches the single-backend-
    // parameter shape. Using a non-inline variable template keeps the
    // checks together and compile-time.
    template<class T>
    inline constexpr bool is_complete_v = sizeof(T) > 0;

    #define ONYXUI_TYPE(name) static_assert(is_complete_v<name>);
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

    // Sanity: the primary `onyxui::button<B>` template in the root
    // onyxui namespace is NOT shadowed by our aliases — they live in
    // the sub-namespace above.
    static_assert(std::is_class_v<::onyxui::button<backend>>);

} // namespace test_backend_aliases

TEST_CASE("backend aliases — X-macro mechanism holds against test_backend") {
    using namespace test_backend_aliases;

    ui_host host;
    auto root = std::make_unique<vbox>();
    host.mount(std::move(root));
    CHECK(host.root() != nullptr);
}

// --------------------------------------------------------------------
// Layer 2: sdlpp shell header smoke — gated by availability
// --------------------------------------------------------------------

#if __has_include(<onyxui/sdlpp/sdlpp_backend.hh>)
#  include <onyxui/backend/sdlpp.hh>

TEST_CASE("backend aliases (sdlpp) — ui_host + vbox via aliased names") {
    using namespace ::onyxui::sdlpp;

    ui_host host;
    auto root = std::make_unique<vbox>();
    host.mount(std::move(root));
    CHECK(host.root() != nullptr);
}
#endif

// --------------------------------------------------------------------
// Layer 3: conio shell header smoke — gated by availability
// --------------------------------------------------------------------

#if __has_include(<onyxui/conio/conio_backend.hh>)
#  include <onyxui/backend/conio.hh>

TEST_CASE("backend aliases (conio) — ui_host + vbox via aliased names") {
    using namespace ::onyxui::conio;

    ui_host host;
    auto root = std::make_unique<vbox>();
    host.mount(std::move(root));
    CHECK(host.root() != nullptr);
}
#endif
