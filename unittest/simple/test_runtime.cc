/// @file test_runtime.cc
/// @brief Tests for the simple-shell live-dialog + app_window
///        registry.
///
/// runtime.cc is backend-agnostic (zero sdlpp dependencies) — it's
/// compiled directly into `ui_unittest` as a source, not linked
/// from onyxui_sdlpp_backend, so these tests work in every build
/// configuration regardless of which backends are enabled. The
/// test uses fake `app_window*` pointers (identity-only) and
/// never touches SDL.

#include <doctest/doctest.h>

#include <onyxui/simple/detail/runtime.hh>

#include <cstddef>
#include <memory>
#include <vector>

// Forward-declared in detail/runtime.hh; the tests only use pointer
// identity so no definition is needed.
namespace onyxui::simple {
    // app_window is forward-declared in detail/runtime.hh.
}

namespace {
    // Placeholder types whose addresses are used as identity keys.
    struct fake_owner {};
    struct fake_dialog {};

    // Helpers to reduce reinterpret_cast noise in the tests.
    using namespace onyxui::simple;
    using namespace onyxui::simple::detail;

    app_window* as_owner(fake_owner& f) {
        return reinterpret_cast<app_window*>(&f);
    }
}

TEST_CASE("simple::detail — register + dismiss round-trip") {
    fake_owner owner;
    fake_dialog dlg_a;
    fake_dialog dlg_b;

    // The registry is thread-local — cleanse any residue from an
    // earlier test (there shouldn't be any, but be defensive).
    dismiss_dialogs_for(as_owner(owner));
    REQUIRE(live_dialog_count() == 0);

    bool a_fired = false;
    bool b_fired = false;
    register_live_dialog(as_owner(owner), &dlg_a, [&] { a_fired = true; });
    register_live_dialog(as_owner(owner), &dlg_b, [&] { b_fired = true; });
    CHECK(live_dialog_count() == 2);

    dismiss_live_dialog(&dlg_a);
    CHECK(a_fired);
    CHECK_FALSE(b_fired);
    CHECK(live_dialog_count() == 1);

    dismiss_live_dialog(&dlg_b);
    CHECK(b_fired);
    CHECK(live_dialog_count() == 0);
}

TEST_CASE("simple::detail — dismiss_dialogs_for drops every entry for an owner") {
    fake_owner owner_a;
    fake_owner owner_b;
    fake_dialog dlg_a1, dlg_a2, dlg_b1;

    dismiss_dialogs_for(as_owner(owner_a));
    dismiss_dialogs_for(as_owner(owner_b));
    REQUIRE(live_dialog_count() == 0);

    std::vector<int> fired;
    register_live_dialog(as_owner(owner_a), &dlg_a1, [&] { fired.push_back(1); });
    register_live_dialog(as_owner(owner_a), &dlg_a2, [&] { fired.push_back(2); });
    register_live_dialog(as_owner(owner_b), &dlg_b1, [&] { fired.push_back(3); });
    CHECK(live_dialog_count() == 3);

    // Tearing down owner_a must drop both of its dialogs — and
    // only those — leaving owner_b untouched. This is the path
    // that app_window::close() / ~app_window() take.
    dismiss_dialogs_for(as_owner(owner_a));
    CHECK(fired.size() == 2);
    CHECK((fired[0] == 1 && fired[1] == 2));
    CHECK(live_dialog_count() == 1);

    // Cleanup.
    dismiss_dialogs_for(as_owner(owner_b));
    CHECK(live_dialog_count() == 0);
}

TEST_CASE("simple::detail — dismiss_live_dialog is safe when called from inside a disposer") {
    // Mirrors the real dialog flow: the window's `closed` signal
    // handler calls `dismiss_live_dialog(raw)`, and the disposer
    // (which drops the presenter → destroys the window) may ALSO
    // run `closed` logic that recurses into dismiss_live_dialog.
    // The registry removes the entry BEFORE invoking the disposer,
    // so recursion is harmless.
    fake_owner owner;
    fake_dialog dlg;

    dismiss_dialogs_for(as_owner(owner));
    REQUIRE(live_dialog_count() == 0);

    int call_count = 0;
    register_live_dialog(
        as_owner(owner), &dlg,
        [&] {
            ++call_count;
            // Simulate the dialog's close-emitting signal handler
            // re-entering dismiss_live_dialog. Must not cause the
            // disposer to run again.
            dismiss_live_dialog(&dlg);
        });
    dismiss_live_dialog(&dlg);
    CHECK(call_count == 1);
    CHECK(live_dialog_count() == 0);
}

TEST_CASE("simple::detail — dismiss_dialogs_for runs disposers after removing entries") {
    // The disposer may want to inspect the registry (e.g., via
    // live_dialog_count) during teardown. At that point its own
    // entry should already be gone — that's what allows recursive
    // dismissal calls from inside the disposer to no-op safely.
    fake_owner owner;
    fake_dialog dlg;

    dismiss_dialogs_for(as_owner(owner));
    REQUIRE(live_dialog_count() == 0);

    std::size_t count_seen_inside_disposer = 42;
    register_live_dialog(
        as_owner(owner), &dlg,
        [&] {
            count_seen_inside_disposer = live_dialog_count();
        });
    CHECK(live_dialog_count() == 1);

    dismiss_dialogs_for(as_owner(owner));
    CHECK(count_seen_inside_disposer == 0);  // entry removed before disposer fires
    CHECK(live_dialog_count() == 0);
}
