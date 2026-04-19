/// @file test_ui_host.cc
/// @brief Tests for `ui_host<B>` — the consolidated embedding type.

#include <doctest/doctest.h>

#include "utils/test_backend.hh"
#include "utils/test_helpers.hh"

#include <onyxui/services/ui_services.hh>
#include <onyxui/ui_host.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/window/window.hh>

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_backend;

namespace {

    // Build a minimal root widget (a label inside a vbox).
    std::unique_ptr<ui_element<Backend>> make_root() {
        auto root = std::make_unique<vbox<Backend>>();
        root->set_width_constraint({size_policy::fixed, logical_unit(200.0)});
        root->set_height_constraint({size_policy::fixed, logical_unit(100.0)});
        return root;
    }

    // Build a minimal overlay window.
    std::unique_ptr<window<Backend>> make_dialog(const char* title = "Dlg") {
        typename window<Backend>::window_flags flags;
        auto w = std::make_unique<window<Backend>>(title, flags);
        w->set_width_constraint({size_policy::fixed, logical_unit(120.0)});
        w->set_height_constraint({size_policy::fixed, logical_unit(60.0)});
        return w;
    }

} // anonymous namespace

// ============================================================================
// Construction / destruction
// ============================================================================

TEST_CASE("ui_host - default construction works without a mounted root") {
    ui_host<Backend> host;
    CHECK(host.root() == nullptr);
    // Services are reachable via escape hatches.
    (void)host.layers();
    (void)host.themes();
    (void)host.hotkeys();
    (void)host.input();
    (void)host.metrics();
}

TEST_CASE("ui_host - non-copyable, non-moveable") {
    static_assert(!std::is_copy_constructible_v<ui_host<Backend>>);
    static_assert(!std::is_copy_assignable_v<ui_host<Backend>>);
    static_assert(!std::is_move_constructible_v<ui_host<Backend>>);
    static_assert(!std::is_move_assignable_v<ui_host<Backend>>);
    CHECK(true);
}

TEST_CASE("ui_host - mount / unmount round-trip") {
    ui_host<Backend> host;
    CHECK(host.root() == nullptr);

    host.mount(make_root());
    auto* mounted = host.root();
    REQUIRE(mounted != nullptr);

    auto taken = host.unmount();
    CHECK(host.root() == nullptr);
    CHECK(taken.get() == mounted);
}

TEST_CASE("ui_host - mount replaces any previous root") {
    ui_host<Backend> host;
    host.mount(make_root());
    auto* first = host.root();
    REQUIRE(first != nullptr);

    host.mount(make_root());
    CHECK(host.root() != nullptr);
    CHECK(host.root() != first);  // previous root destroyed
}

// ============================================================================
// Overlay presentation
// ============================================================================

TEST_CASE("ui_host - present() returns a valid non-modal presenter") {
    ui_host<Backend> host;
    host.mount(make_root());

    const int before = host.layers().layer_count();
    auto presenter = host.present(make_dialog("modeless"));
    CHECK(static_cast<bool>(presenter));
    CHECK(presenter.get() != nullptr);
    CHECK_FALSE(presenter->is_modal());
    CHECK(host.layers().layer_count() == before + 1);
}

TEST_CASE("ui_host - present_modal() returns a valid modal presenter") {
    ui_host<Backend> host;
    host.mount(make_root());

    const int before = host.layers().layer_count();
    auto presenter = host.present_modal(make_dialog("modal"));
    CHECK(static_cast<bool>(presenter));
    CHECK(presenter.get() != nullptr);
    CHECK(presenter->is_modal());
    CHECK(host.layers().layer_count() == before + 1);
}

TEST_CASE("ui_host - dropping a presenter removes its layer") {
    ui_host<Backend> host;
    host.mount(make_root());

    const int before = host.layers().layer_count();
    {
        auto p = host.present(make_dialog());
        CHECK(host.layers().layer_count() == before + 1);
    }
    CHECK(host.layers().layer_count() == before);
}

// ============================================================================
// unmount() + live overlays (§7.1)
// ============================================================================

TEST_CASE("ui_host - unmount does not dismiss live overlays") {
    ui_host<Backend> host;
    host.mount(make_root());

    auto presenter = host.present(make_dialog());
    const int with_overlay = host.layers().layer_count();
    CHECK(with_overlay >= 1);

    // Unmount the root. Overlays must stay alive.
    auto taken = host.unmount();
    CHECK(host.root() == nullptr);
    CHECK(host.layers().layer_count() == with_overlay);
    CHECK(static_cast<bool>(presenter));
    CHECK(presenter.get() != nullptr);

    // Re-mount a different root — overlays still unaffected.
    host.mount(make_root());
    CHECK(host.root() != nullptr);
    CHECK(host.layers().layer_count() == with_overlay);
    CHECK(static_cast<bool>(presenter));
}

TEST_CASE("ui_host - overlay outlives mount/unmount churn") {
    ui_host<Backend> host;
    auto presenter = host.present_modal(make_dialog("persistent"));
    REQUIRE(static_cast<bool>(presenter));

    // Mount, unmount, mount, unmount — presenter must stay valid.
    for (int i = 0; i < 4; ++i) {
        host.mount(make_root());
        auto taken = host.unmount();
        CHECK(static_cast<bool>(presenter));
    }
    CHECK(host.layers().layer_count() >= 1);
}

// ============================================================================
// Render
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - render with a mounted root invokes measure+arrange+draw") {
    // Nested inside the fixture's context so a theme is registered.
    ui_host<Backend> host;
    host.mount(make_root());

    typename Backend::renderer_type r{};
    const logical_rect viewport{0.0_lu, 0.0_lu, 200.0_lu, 100.0_lu};

    // Must not throw; should produce non-zero bounds on the root.
    host.render(r, viewport);

    auto* root = host.root();
    REQUIRE(root != nullptr);
    CHECK(root->bounds().width.value > 0.0);
    CHECK(root->bounds().height.value > 0.0);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - render with no mounted root is a no-op (renders overlays only)") {
    ui_host<Backend> host;
    typename Backend::renderer_type r{};
    const logical_rect viewport{0.0_lu, 0.0_lu, 100.0_lu, 100.0_lu};

    // Should not throw even though no root is mounted.
    host.render(r, viewport);

    // With a live overlay but no root, render should still be safe
    // (the ghost-overlay case documented in §7.1).
    auto presenter = host.present(make_dialog());
    host.render(r, viewport);
    CHECK(static_cast<bool>(presenter));
}

// ============================================================================
// Destruction with live overlays
// ============================================================================

// ============================================================================
// Dormancy between calls (§7 of the design)
// ============================================================================

TEST_CASE("ui_host - host is dormant between render/event calls") {
    ui_host<Backend> host;

    // Outside any call, there must be no ambient context for this
    // backend — the host does NOT push on construction.
    CHECK(ui_services<Backend>::layers() == nullptr);
    CHECK(ui_services<Backend>::themes() == nullptr);
    CHECK(ui_services<Backend>::input() == nullptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - scope is pushed inside render and popped after") {
    // The fixture has already pushed its own context; that's our
    // baseline. ui_host must push on top and pop back to it.
    auto* outer_layers = ui_services<Backend>::layers();
    REQUIRE(outer_layers != nullptr);

    ui_host<Backend> host;
    host.mount(make_root());

    // Nothing pushed yet (host is dormant).
    CHECK(ui_services<Backend>::layers() == outer_layers);

    typename Backend::renderer_type r{};
    host.render(r, logical_rect{0.0_lu, 0.0_lu, 100.0_lu, 100.0_lu});

    // After render returns, the scope guard has popped; we're back
    // to the outer context.
    CHECK(ui_services<Backend>::layers() == outer_layers);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - two hosts on one thread do not fight for ambient slot") {
    auto* outer_layers = ui_services<Backend>::layers();
    REQUIRE(outer_layers != nullptr);

    ui_host<Backend> host_a;
    ui_host<Backend> host_b;

    // Neither host is "ambient" between calls — the outer context
    // (the fixture's) stays on top. Critical: constructing host_b
    // doesn't shadow host_a.
    CHECK(ui_services<Backend>::layers() == outer_layers);

    // host_a.layers() returns host_a's own layer_manager (not ambient).
    CHECK(&host_a.layers() != outer_layers);
    CHECK(&host_b.layers() != outer_layers);
    CHECK(&host_a.layers() != &host_b.layers());

    // When host_a is inside a call, ambient resolves to host_a's
    // layers — not host_b's.
    typename Backend::renderer_type r{};
    host_a.mount(make_root());
    host_a.render(r, logical_rect{0.0_lu, 0.0_lu, 50.0_lu, 50.0_lu});
    // After the call returns, host_a is dormant again.
    CHECK(ui_services<Backend>::layers() == outer_layers);
}

// ============================================================================
// Viewport origin (sub-viewport hosting)
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - render honors the viewport origin") {
    ui_host<Backend> host;

    // Give the root a fixed intrinsic size that measure/arrange will
    // realize at the arranged origin.
    auto root = std::make_unique<vbox<Backend>>();
    root->set_width_constraint({size_policy::fixed, logical_unit(80.0)});
    root->set_height_constraint({size_policy::fixed, logical_unit(40.0)});
    auto* root_ptr = root.get();
    host.mount(std::move(root));

    typename Backend::renderer_type r{};
    // Ask the host to place the root inside a viewport that does NOT
    // start at the origin.
    const logical_rect viewport{10.0_lu, 20.0_lu, 80.0_lu, 40.0_lu};
    host.render(r, viewport);

    // The root must be arranged at the viewport's origin, not (0, 0).
    const auto& bounds = root_ptr->bounds();
    CHECK(bounds.x.value == doctest::Approx(10.0));
    CHECK(bounds.y.value == doctest::Approx(20.0));
    CHECK(bounds.width.value == doctest::Approx(80.0));
    CHECK(bounds.height.value == doctest::Approx(40.0));
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - render(renderer) overload honors the renderer's physical origin") {
    ui_host<Backend> host;

    auto root = std::make_unique<vbox<Backend>>();
    root->set_width_constraint({size_policy::fixed, logical_unit(50.0)});
    root->set_height_constraint({size_policy::fixed, logical_unit(30.0)});
    auto* root_ptr = root.get();
    host.mount(std::move(root));

    // Configure the test_backend renderer to report a viewport whose
    // origin is NOT at (0, 0). The convenience overload must propagate
    // that origin through to the arrange() call.
    typename Backend::renderer_type r{};
    r.viewport_override = typename Backend::rect_type{7, 11, 50, 30};

    host.render(r);

    const auto& bounds = root_ptr->bounds();
    CHECK(bounds.x.value == doctest::Approx(7.0));
    CHECK(bounds.y.value == doctest::Approx(11.0));
    CHECK(bounds.width.value == doctest::Approx(50.0));
    CHECK(bounds.height.value == doctest::Approx(30.0));
}

// ============================================================================
// present() / present_modal() scope behavior
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - present_modal pushes scope so show_modal sees host's window_manager") {
    // window::show_modal consults ambient ui_services::windows() to
    // set the active window. If present_modal fails to push the host's
    // scope before calling show, the active-window lookup either
    // misses entirely or targets the fixture's outer window_manager
    // (which is NOT the host's).
    ui_host<Backend> host;
    host.mount(make_root());

    // Before: the host's window_manager has no active window for
    // the dialog we're about to present.
    CHECK(host.windows().get_active_window() == nullptr);

    auto presenter = host.present_modal(make_dialog("modal"));
    REQUIRE(static_cast<bool>(presenter));

    // After present_modal: the host's OWN window_manager records the
    // modal as active. This is only possible if the scope was pushed
    // during the call — otherwise ui_services::windows() would have
    // returned the fixture's window_manager or nullptr.
    CHECK(host.windows().get_active_window() == presenter.get());

    // And dormancy is restored after the call: the ambient pointer
    // is the fixture's again, not the host's.
    CHECK(ui_services<Backend>::windows() != &host.windows());
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "ui_host - present pushes scope (ambient points at host during call, restored after)") {
    // For non-modal present(), the observable effect is a layer
    // registered with the host's layers. The scope-push invariant
    // is verifiable by dormancy-after-call: before and after the
    // call, ambient layers is the fixture's, not the host's.
    auto* outer_layers = ui_services<Backend>::layers();
    REQUIRE(outer_layers != nullptr);

    ui_host<Backend> host;
    CHECK(&host.layers() != outer_layers);

    auto presenter = host.present(make_dialog());
    REQUIRE(static_cast<bool>(presenter));

    // The overlay landed on the host's layers, not the outer one.
    CHECK(host.layers().layer_count() >= 1);
    CHECK(outer_layers->layer_count() == 0);

    // Ambient is restored to the fixture's after present() returns.
    CHECK(ui_services<Backend>::layers() == outer_layers);
}

TEST_CASE("ui_host - destruction with live overlays is safe (layer_manager teardown)") {
    // Heap-allocate the presenter too so we can sequence its destruction
    // AFTER the host. The presenter's underlying window observes the
    // layer_manager's destroying signal (shipped in WAR-45 via
    // window::m_layer_owner_conn) and degrades safely.
    auto host = std::make_unique<ui_host<Backend>>();
    auto presenter = std::make_unique<presented_window<Backend>>(
        host->present(make_dialog("outlives-host")));
    REQUIRE(static_cast<bool>(*presenter));

    // Tear down the host first.
    host.reset();

    // Dropping the presenter afterwards must not UAF — the window's
    // destructor no-ops the layer-removal path because the owning
    // manager is already gone.
    presenter.reset();
    CHECK(true);  // Reaching here without a crash IS the assertion.
}
