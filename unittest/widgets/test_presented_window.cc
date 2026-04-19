/// @file test_presented_window.cc
/// @brief Tests for the RAII overlay-presentation handle.

#include <doctest/doctest.h>

#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/presented_window.hh>

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_backend;

namespace {
    std::unique_ptr<window<Backend>> make_test_window(const char* title = "Test") {
        typename window<Backend>::window_flags flags;
        auto w = std::make_unique<window<Backend>>(title, flags);
        w->set_width_constraint({size_policy::fixed, logical_unit(100.0)});
        w->set_height_constraint({size_policy::fixed, logical_unit(50.0)});
        return w;
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "presented_window shows the window on construction, hides on destruction") {
    layer_manager<Backend> layers;

    {
        presented_window<Backend> p(layers, make_test_window());
        CHECK(p.is_visible());
        CHECK(p.get() != nullptr);
        CHECK(layers.layer_count() == 1);
    } // p destructed here

    CHECK(layers.layer_count() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "presented_window modal vs modeless") {
    layer_manager<Backend> layers;

    SUBCASE("modal presentation") {
        presented_window<Backend> p(layers, make_test_window(),
                                    presentation_kind::modal);
        CHECK(p->is_modal());
        CHECK(layers.layer_count() == 1);
    }

    SUBCASE("modeless presentation") {
        presented_window<Backend> p(layers, make_test_window(),
                                    presentation_kind::modeless);
        CHECK_FALSE(p->is_modal());
        CHECK(layers.layer_count() == 1);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "presented_window move-constructs, moved-from is harmless") {
    layer_manager<Backend> layers;

    auto p1 = std::make_unique<presented_window<Backend>>(layers, make_test_window());
    CHECK(layers.layer_count() == 1);

    // Move into a new handle — source becomes no-op.
    presented_window<Backend> p2(std::move(*p1));
    CHECK(p2.get() != nullptr);
    CHECK_FALSE(static_cast<bool>(*p1));  // moved-from
    CHECK(layers.layer_count() == 1);     // still one layer — just moved

    // Destroy the moved-from handle — must be a no-op (no double-remove).
    p1.reset();
    CHECK(layers.layer_count() == 1);

    // Destroy p2 — the layer is finally removed.
    {
        presented_window<Backend> sink = std::move(p2);
        CHECK(sink.get() != nullptr);
    }
    CHECK(layers.layer_count() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "presented_window move-assign replaces the previous window") {
    layer_manager<Backend> layers;

    presented_window<Backend> p(layers, make_test_window("first"));
    CHECK(p->get_title() == "first");
    CHECK(layers.layer_count() == 1);

    p = presented_window<Backend>(layers, make_test_window("second"));
    CHECK(p->get_title() == "second");
    CHECK(layers.layer_count() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "presented_window survives the manager going away first") {
    // If the layer_manager outlives the presenter, normal teardown
    // happens. If the manager is destroyed first, window::hide() must
    // observe the destruction via m_layer_owner_conn and no-op safely.
    std::unique_ptr<presented_window<Backend>> p;
    {
        layer_manager<Backend> transient;
        p = std::make_unique<presented_window<Backend>>(
            transient, make_test_window());
        CHECK(transient.layer_count() == 1);
    }
    // transient is destroyed here; its destroying signal fires, window
    // clears m_layer_owner. Destructing p should not crash.
    p.reset();
    CHECK(true);  // reaching here without UAF is the assertion
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "presented_window::release returns the window without hiding") {
    layer_manager<Backend> layers;

    presented_window<Backend> p(layers, make_test_window());
    CHECK(layers.layer_count() == 1);

    auto w = p.release();
    CHECK(w != nullptr);
    CHECK_FALSE(static_cast<bool>(p));

    // Ownership transferred to `w`. Layer is still registered — the
    // release doesn't tear it down; caller is now responsible.
    CHECK(layers.layer_count() == 1);

    // Hide the transferred window explicitly to leave things tidy.
    w->hide(layers);
    CHECK(layers.layer_count() == 0);
}
