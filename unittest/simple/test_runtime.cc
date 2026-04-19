/// @file test_runtime.cc
/// @brief Tests for the simple-shell run-loop registry.
///
/// The state under test here (window registry + quit flag) lives in
/// `inline thread_local` variables in `<onyxui/simple/detail/runtime.hh>`.
/// The header is backend-agnostic, so these tests compile against
/// `ui_unittest` without linking any backend implementation. Tests
/// use fake `app_window*` pointers (identity-only — no OS window is
/// created).
///
/// Modal-dialog ownership is deliberately NOT tested here: after the
/// Option-D refactor, modal presenters are stored per-`app_window`
/// rather than in a thread-local registry, so they can only be
/// exercised through a live `app_window` + ui_host. That requires a
/// backend and belongs with integration tests.

#include <doctest/doctest.h>

#include <onyxui/simple/detail/runtime.hh>

#include <cstddef>
#include <vector>

namespace {
    // Placeholder type whose address is used as an identity key.
    struct fake_app_window {};

    using namespace onyxui::simple;
    using namespace onyxui::simple::detail;

    app_window* as_ptr(fake_app_window& f) {
        return reinterpret_cast<app_window*>(&f);
    }

    // Clean slate helper — later tests may run in the same thread.
    void reset_registry() {
        // No public "clear" — unregister every snapshot entry.
        std::vector<app_window*> all;
        for_each_registered_window(
            [](app_window* w, void* userdata) {
                static_cast<std::vector<app_window*>*>(userdata)
                    ->push_back(w);
            },
            &all);
        for (auto* w : all) unregister_window(w);
        reset_quit();
    }
}

TEST_CASE("simple::detail — register / unregister round-trip") {
    reset_registry();
    REQUIRE(registered_window_count() == 0);

    fake_app_window a, b;
    register_window(as_ptr(a));
    register_window(as_ptr(b));
    CHECK(registered_window_count() == 2);

    // Re-registering the same window is a no-op.
    register_window(as_ptr(a));
    CHECK(registered_window_count() == 2);

    unregister_window(as_ptr(a));
    CHECK(registered_window_count() == 1);

    unregister_window(as_ptr(b));
    CHECK(registered_window_count() == 0);

    // Unregister of an absent entry is a no-op.
    unregister_window(as_ptr(a));
    CHECK(registered_window_count() == 0);
}

TEST_CASE("simple::detail — for_each_registered_window snapshots, "
          "so callbacks can safely unregister mid-iteration") {
    reset_registry();

    fake_app_window a, b, c;
    register_window(as_ptr(a));
    register_window(as_ptr(b));
    register_window(as_ptr(c));

    // The callback unregisters each window as it sees it — a naive
    // iteration would skip entries, but the snapshot lets us drain
    // the registry in one pass.
    std::vector<app_window*> seen;
    for_each_registered_window(
        [](app_window* w, void* userdata) {
            static_cast<std::vector<app_window*>*>(userdata)->push_back(w);
            unregister_window(w);
        },
        &seen);

    CHECK(seen.size() == 3);
    CHECK(registered_window_count() == 0);
}

TEST_CASE("simple::detail — quit flag round-trip") {
    reset_registry();
    CHECK_FALSE(quit_requested());
    CHECK(exit_code() == 0);

    request_quit(7);
    CHECK(quit_requested());
    CHECK(exit_code() == 7);

    reset_quit();
    CHECK_FALSE(quit_requested());
    CHECK(exit_code() == 0);
}

TEST_CASE("simple::detail — null / no-op inputs are tolerated") {
    reset_registry();

    register_window(nullptr);
    CHECK(registered_window_count() == 0);

    unregister_window(nullptr);
    CHECK(registered_window_count() == 0);

    for_each_registered_window(nullptr, nullptr);  // no callback
    CHECK(registered_window_count() == 0);
}
