/// @file test_window_show.cc
/// @brief Covers the non-modal window::show() positioning behavior.
///
/// A freshly-constructed window has default bounds (0,0,0,0). Historically
/// show() forwarded those to the layer manager verbatim, which meant the
/// window arranged at zero size and rendered invisibly. Callers had to use
/// show_modal() to get auto-centering. The current contract:
///
///  - show() with explicit bounds (set_position/set_size beforehand) keeps
///    those bounds.
///  - show() with zero bounds delegates to the layer manager's auto-position
///    path, matching show_modal_dialog's behavior.

#include <doctest/doctest.h>

#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

#include <onyxui/widgets/window/window.hh>

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_backend;

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "window::show() auto-centers when bounds were not pre-set") {
    auto win = std::make_unique<window<Backend>>("Test");
    win->set_width_constraint({size_policy::fixed, logical_unit(200.0)});
    win->set_height_constraint({size_policy::fixed, logical_unit(100.0)});
    // Intentionally do NOT call set_position/set_size. Bounds stay zero.

    win->show();

    // Render once so the layer manager runs its auto-positioning pass.
    const typename Backend::rect_type viewport{0, 0, 800, 600};

    auto* layers = ui_services<Backend>::layers();
    REQUIRE(layers != nullptr);
    auto* themes = ui_services<Backend>::themes();
    REQUIRE(themes != nullptr);
    auto* theme = themes->get_current_theme();
    REQUIRE(theme != nullptr);
    auto* metrics_ptr = ui_services<Backend>::metrics();
    REQUIRE(metrics_ptr != nullptr);

    // Grab a renderer from the fixture's context to hand to render_all_layers.
    auto r = typename Backend::renderer_type{};

    layers->render_all_layers(r, viewport, theme, *metrics_ptr);

    // After the render pass, the window should have non-zero bounds roughly
    // centered in the viewport (200×100 in 800×600 → x≈300, y≈250).
    const auto b = win->bounds();
    CHECK(b.width.value == doctest::Approx(200.0));
    CHECK(b.height.value == doctest::Approx(100.0));
    CHECK(b.x.value > 0.0);
    CHECK(b.y.value > 0.0);
    // Specifically centered:
    CHECK(b.x.value == doctest::Approx(300.0));
    CHECK(b.y.value == doctest::Approx(250.0));
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "window::show() across two managers doesn't orphan the old layer") {
    // If a caller shows the window on manager A and then calls show/hide
    // against manager B, the window must first clean up its registration
    // from A — otherwise the old manager keeps a dead entry.
    auto win = std::make_unique<window<Backend>>("Test");
    win->set_width_constraint({size_policy::fixed, logical_unit(100.0)});
    win->set_height_constraint({size_policy::fixed, logical_unit(50.0)});

    layer_manager<Backend> a;
    layer_manager<Backend> b;

    const typename Backend::rect_type viewport{0, 0, 400, 300};
    auto* themes = ui_services<Backend>::themes();
    auto* theme = themes ? themes->get_current_theme() : nullptr;
    auto* metrics_ptr = ui_services<Backend>::metrics();
    REQUIRE(theme != nullptr);
    REQUIRE(metrics_ptr != nullptr);

    SUBCASE("Second show() on a different manager migrates cleanly") {
        win->show(a);
        CHECK(a.layer_count() == 1);

        win->show(b);
        // The layer in `a` must have been removed; `b` now holds it.
        CHECK(a.layer_count() == 0);
        CHECK(b.layer_count() == 1);
    }

    SUBCASE("hide() on the wrong manager still removes from the real owner") {
        win->show(a);
        CHECK(a.layer_count() == 1);

        // Caller passes B by mistake; implementation should notice the
        // real owner is A and remove from there.
        win->hide(b);
        CHECK(a.layer_count() == 0);
        CHECK(b.layer_count() == 0);
    }

    SUBCASE("show() after hide() registers in the new manager") {
        win->show(a);
        win->hide(a);
        CHECK(a.layer_count() == 0);

        win->show(b);
        CHECK(a.layer_count() == 0);
        CHECK(b.layer_count() == 1);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "window::hide() after the owning layer_manager is destroyed doesn't UAF") {
    // If the layer_manager is destroyed before the window (e.g. a scoped
    // ui_context torn down before a dialog holding the window), the
    // window's `m_layer_owner` must be cleared automatically. Otherwise
    // window::hide() — called from the destructor — would dereference a
    // dangling pointer.
    auto win = std::make_unique<window<Backend>>("Test");
    win->set_width_constraint({size_policy::fixed, logical_unit(100.0)});
    win->set_height_constraint({size_policy::fixed, logical_unit(50.0)});

    {
        layer_manager<Backend> transient;
        win->show(transient);
        // transient goes out of scope here — its destroying signal must
        // fire and clear the window's owner pointer.
    }

    // If the back-pointer weren't cleared, this would crash.
    win->hide();
    CHECK(!win->is_visible());
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "window::show() preserves explicitly-set bounds") {
    auto win = std::make_unique<window<Backend>>("Test");
    win->set_width_constraint({size_policy::fixed, logical_unit(200.0)});
    win->set_height_constraint({size_policy::fixed, logical_unit(100.0)});
    win->set_position(50, 40);
    win->set_size(200, 100);

    win->show();

    const typename Backend::rect_type viewport{0, 0, 800, 600};
    auto* layers = ui_services<Backend>::layers();
    REQUIRE(layers != nullptr);
    auto* themes = ui_services<Backend>::themes();
    REQUIRE(themes != nullptr);
    auto* theme = themes->get_current_theme();
    REQUIRE(theme != nullptr);
    auto* metrics_ptr = ui_services<Backend>::metrics();
    REQUIRE(metrics_ptr != nullptr);

    auto r = typename Backend::renderer_type{};
    layers->render_all_layers(r, viewport, theme, *metrics_ptr);

    const auto b = win->bounds();
    // Position honored; size reflects set_size.
    CHECK(b.x.value == doctest::Approx(50.0));
    CHECK(b.y.value == doctest::Approx(40.0));
    CHECK(b.width.value == doctest::Approx(200.0));
    CHECK(b.height.value == doctest::Approx(100.0));
}
