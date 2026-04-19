/// @file test_ui_host.cc
/// @brief Tests for `ui_host<B>` — the consolidated embedding type.

#include <doctest/doctest.h>

#include "utils/test_backend.hh"
#include "utils/test_helpers.hh"

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
