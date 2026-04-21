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

// Pull in the canonical include set for every aliased type — keeps
// this test in lockstep with the registry without a manual list to
// maintain.
#include <onyxui/detail/type_registry.inc>

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

// Gate the sdlpp block on ALL headers in the chain being present —
// `onyxui/sdlpp/sdlpp_backend.hh` may be reachable even when the
// SDL++ library itself isn't, which would let the first include
// succeed and then fail on `sdlpp/events/events.hh`.
#if __has_include(<onyxui/sdlpp/sdlpp_backend.hh>) && \
    __has_include(<sdlpp/events/events.hh>)
#  include <onyxui/backend/sdlpp.hh>

TEST_CASE("backend aliases (sdlpp) — ui_host + vbox via aliased names") {
    using namespace ::onyxui::sdlpp;

    ui_host host;
    auto root = std::make_unique<vbox>();
    host.mount(std::move(root));
    CHECK(host.root() != nullptr);
}

// Bundle header: the single public entry point for simple-shell
// consumers. Pulls in app_window, run/quit, and (once WAR-55
// lands) the dialog helpers. Smoke test ensures the header
// resolves every name it needs after the X-macro promotion step.
#  include <onyxui/for/sdlpp.hh>

TEST_CASE("simple-shell bundle (sdlpp) — app_window declarations resolve") {
    // We deliberately don't CONSTRUCT an app_window here (doing so
    // would initialise SDL and open a real OS window, which is wrong
    // for a unit test). What we pin is that:
    //   - the bundle header compiles,
    //   - onyxui::simple::app_window / ui_host / ui_element / run /
    //     quit are all declared at the API names the docs promise.
    using simple_app_window = ::onyxui::simple::app_window;
    static_assert(sizeof(simple_app_window) > 0);

    using simple_ui_host = ::onyxui::simple::ui_host;
    static_assert(sizeof(simple_ui_host) > 0);

    // quit() is a free function; take its address as a syntactic
    // check — the signature is void(int) noexcept.
    [[maybe_unused]] void (*quit_ptr)(int) noexcept =
        &::onyxui::simple::quit;
    [[maybe_unused]] int (*run_ptr)() = &::onyxui::simple::run;
    CHECK(quit_ptr != nullptr);
    CHECK(run_ptr != nullptr);
}
#endif

// --------------------------------------------------------------------
// Layer 3: conio shell header smoke — gated by availability
// --------------------------------------------------------------------

#if __has_include(<onyxui/conio/conio_backend.hh>) && \
    __has_include(<termbox2.h>)
#  include <onyxui/backend/conio.hh>

TEST_CASE("backend aliases (conio) — ui_host + vbox via aliased names") {
    using namespace ::onyxui::conio;

    ui_host host;
    auto root = std::make_unique<vbox>();
    host.mount(std::move(root));
    CHECK(host.root() != nullptr);
}
#endif
