# OnyxUI Simple Shell Design

> Status: proposal (2026-04-19)
> Companion to:
> - [ONYXUI_RUNTIME_DIRECTION.md](./ONYXUI_RUNTIME_DIRECTION.md) (adopted)
> - [ONYXUI_UI_HOST_DESIGN.md](./ONYXUI_UI_HOST_DESIGN.md) (proposal)
> Scope: the "FLTK-simple" consumer API for standalone tools and
> the backend-pick mechanism that both the simple shell and direct
> `ui_host<B>` consumers use
> Non-scope: engine embedding (that's `ui_host<B>`), runtime
> backend switching (not a thing; see §5)

---

## 1. Why this exists

The embedding runtime story is handled by `ui_host<B>`. It's the
right answer for engines and custom hosts. It is not the right
answer for a newcomer who has seen OnyxUI on GitHub and wants to
know if they should use it.

A newcomer's first experience should be:

```cpp
#include <onyxui/for/sdlpp.hh>

int main() {
    onyxui::simple::app_window win("Hello", 640, 480);

    auto root = std::make_unique<onyxui::vbox>();
    root->emplace_child<onyxui::label>("Hello, World!");
    win.set_content(std::move(root));

    win.show();
    return onyxui::simple::run();
}
```

No template parameters. No `scoped_ui_context`. No `backend_metrics`.
No layer manager. No understanding of themes, hotkeys, focus, or
input managers required.

**The acquisition story for OnyxUI depends on this.** A library
that costs a newcomer two hours just to render "Hello, World!" does
not get adopted. One that costs six lines does.

---

## 2. Goals

- **Hello-World in under ten lines**, including `#include` and `main`.
- **Zero template parameters in consumer code.** Backend is picked
  by which shell header you include.
- **One blessed main loop.** `onyxui::simple::run()` pumps events
  and renders until the last `app_window` closes.
- **Standard dialogs as one-liners.** `message_box`, `confirm`,
  `input_dialog` — wraps the existing `dialog<B>` machinery.
- **Compose on `ui_host<B>` internally.** The simple shell is a
  thin veneer; a consumer who outgrows it moves to `ui_host<B>`
  without rewriting widget construction.

## 3. Non-goals

- **Not for engines.** Engines own their render loop and event
  source. The simple shell's `run()` blocks the caller; that's the
  wrong model for engine embedding. Use `ui_host<B>` instead.
- **Not runtime-switchable backends.** A terminal tool never
  becomes a desktop app mid-run. Backend is compile-time-fixed;
  see §5.
- **Not a widget composition DSL.** No builder sugar like
  `vbox().add(label("hi")).add(button("ok"))` in v1. Widgets are
  constructed with the existing `std::make_unique` /
  `emplace_child` patterns. Syntactic sugar may be added later if
  a real consumer asks.
- **Not a replacement for `dialog<B>` or `window_presets.hh`.**
  Those still exist. The simple shell wraps them.

---

## 4. The compile-time backend rule

**Runtime-switchable backends are not a supported concept.** A
binary renders to one target; that target is fixed when the binary
is built. The `B` template parameter in OnyxUI exists so the
library itself can test against multiple backends in one process —
it is library-internal machinery, not a consumer concept.

Consequences:

- Consumer code never writes `<B>`.
- The shell header for a given backend is what makes this true.
- Mixing shell headers in one translation unit is a build error by
  construction (conflicting aliases).

## 5. Per-backend header layout

The backend-alias and simple-shell responsibilities are carried by
two different headers, so engine consumers who want backend
aliases without the simple-shell types can take just the first.

```
<onyxui/backend/sdlpp.hh>   — aliases only (both tiers may use this)
<onyxui/backend/conio.hh>

<onyxui/for/sdlpp.hh>       — aliases + simple-shell bundle (simple tier)
<onyxui/for/conio.hh>
```

### 5.1 Aliases-only header

Aliases go in a per-backend sub-namespace — **not** in `onyxui::`.
Placing them in `onyxui::` with names that match existing class
templates (`using button = button<backend>;`) is a redeclaration
and doesn't compile. A sub-namespace sidesteps that cleanly and
lets the consumer opt in explicitly with `using namespace`.

The canonical list of types that need aliasing is kept in a
single include-file, driven by an X-macro pattern so the same list
expands differently in each consumer (aliases in the backend
sub-namespace, using-declarations in the simple-shell namespace,
conformance-test instantiations, etc.). Adding a new public widget
means editing the list in one place, not each shell header.

```cpp
// include/onyxui/detail/public_types.inc
//
// Canonical list of publicly-templated-on-backend types.
// Included by each shell header with ONYXUI_TYPE redefined per
// expansion site. This file has no header guard on purpose.

ONYXUI_TYPE(button)
ONYXUI_TYPE(label)
ONYXUI_TYPE(vbox)
ONYXUI_TYPE(hbox)
ONYXUI_TYPE(list_box)
// … every public widget …
ONYXUI_TYPE(window)
ONYXUI_TYPE(presented_window)
ONYXUI_TYPE(ui_host)
```

The per-backend header then uses the list twice — once for the
`backend` typedef, once to expand each alias:

```cpp
// include/onyxui/backend/sdlpp.hh
#pragma once

#include <onyxui/onyxui.hh>
#include <onyxui/backends/sdlpp.hh>

namespace onyxui::sdlpp {

    using backend = ::onyxui::sdlpp_backend;

    #define ONYXUI_TYPE(name) using name = ::onyxui::name<backend>;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

}  // namespace onyxui::sdlpp
```

That expands to:

```cpp
namespace onyxui::sdlpp {
    using backend          = ::onyxui::sdlpp_backend;
    using button           = ::onyxui::button<backend>;
    using label            = ::onyxui::label<backend>;
    using vbox             = ::onyxui::vbox<backend>;
    using hbox             = ::onyxui::hbox<backend>;
    using list_box         = ::onyxui::list_box<backend>;
    // …
    using window           = ::onyxui::window<backend>;
    using presented_window = ::onyxui::presented_window<backend>;
    using ui_host          = ::onyxui::ui_host<backend>;
}
```

Consumer:

```cpp
#include <onyxui/backend/sdlpp.hh>

using namespace onyxui::sdlpp;  // one line, explicit opt-in

int main() {
    ui_host host;
    auto root = std::make_unique<vbox>();
    root->emplace_child<label>("hi");
    host.mount(std::move(root));
    // …
}
```

`using namespace onyxui::sdlpp;` pulls in the backend-fixed
aliases. It does **not** shadow `onyxui::button` (the template),
because the alias lives in a different namespace; both are
reachable by full qualification if anyone ever needs both.

`<onyxui/backend/conio.hh>` mirrors the same structure under
`namespace onyxui::conio`, using the same canonical list.

**Why X-macro + aliases rather than a name-substituting macro:**
aliases preserve IDE tooling (go-to-definition, rename, parameter
hints); a name-replacement macro breaks all of that. The X-macro
generates the aliases once per backend from a single canonical
list; each alias is still a real `using` declaration at the
compiler level, so tooling sees it as a named type.

**Why one header per backend, not an `ONYXUI_BACKEND=sdlpp`
define:** explicit include is greppable and doesn't depend on
compile-flag discipline.

**Canonical-list location:** `include/onyxui/detail/public_types.inc`
is the recommended spot. The `.inc` extension signals "non-standalone
include; re-included with macros redefined". Keeping it under
`detail/` marks it as library machinery — consumers should not
include it directly.

### 5.2 Simple-shell bundle header

The simple-shell bundle includes the aliases header plus the
`onyxui::simple::` types and re-exports the aliases from the
backend sub-namespace into the simple-shell namespace. The same
X-macro list drives it, with a re-export expansion:

```cpp
// include/onyxui/for/sdlpp.hh
#pragma once

#include <onyxui/backend/sdlpp.hh>
#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/run.hh>
#include <onyxui/simple/dialogs.hh>

namespace onyxui::simple {
    using ::onyxui::sdlpp::backend;

    #define ONYXUI_TYPE(name) using ::onyxui::sdlpp::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE

    // app_window, run, quit, message_box, etc. are already
    // declared in the simple/* headers included above.
}
```

That expands to using-declarations that bring each name from
`onyxui::sdlpp::` into `onyxui::simple::` without redeclaration
(using-declarations, not using-directives, so each name is a
standalone introduction).

Consumer:

```cpp
#include <onyxui/for/sdlpp.hh>

using namespace onyxui::simple;

int main() {
    app_window win("Hello", 640, 480);
    auto root = std::make_unique<vbox>();
    root->emplace_child<label>("Hello, World!");
    win.set_content(std::move(root));
    win.show();
    return run();
}
```

### 5.3 warlords and custom backends

A consumer that customizes its backend (warlords' `warlords_backend`
layers renderer adapters on top of sdlpp) can't use the stock
`<onyxui/backend/sdlpp.hh>` header — it needs its own aliases
against its own backend type. warlords already provides the
equivalent today (per-widget typedefs against `warlords_backend`).
The pattern is identical, one layer deeper; no changes needed.

---

## 6. Simple shell API

All types live in `onyxui::simple`. All are non-templated for the
consumer; internally each owns a `ui_host<backend>` (where
`backend` is the alias from the shell header).

## 6.1 `app_window`

A top-level OS window. Owns its OS surface, its renderer, and a
`ui_host<backend>`.

```cpp
namespace onyxui::simple {

class app_window {
public:
    app_window(std::string title, int width, int height);
    ~app_window();

    app_window(const app_window&) = delete;
    app_window& operator=(const app_window&) = delete;
    app_window(app_window&&) = delete;
    app_window& operator=(app_window&&) = delete;

    // Root widget tree.
    void set_content(std::unique_ptr<ui_element<backend>> root);

    // Show/hide the OS window. show() registers the window with the
    // simple::run() loop; close() unregisters and makes run() return
    // once no app_windows remain.
    void show();
    void close();

    // Window chrome state.
    void set_title(const std::string& title);
    [[nodiscard]] bool is_open() const noexcept;

    // Escape hatch — access the underlying ui_host for overlay
    // presentation or for consumers who need a specific service.
    // Most simple-shell consumers never touch this.
    [[nodiscard]] ui_host<backend>& host() noexcept;
};

} // namespace onyxui::simple
```

## 6.2 `run()` / `quit()`

```cpp
namespace onyxui::simple {

// Pump events and render until the last app_window closes or
// quit() is called. Returns the exit code (0 on normal close).
int run();

// Ask the main loop to exit at the next iteration. Does not close
// windows; the loop exits after the current event batch.
void quit(int exit_code = 0);

} // namespace onyxui::simple
```

## 6.3 Dialog helpers

One-liners for the common cases. Each takes the `app_window` it
should be presented on — no auto-find-active-window magic in v1.

```cpp
namespace onyxui::simple {

// Modal "OK" dialog. Returns when the user dismisses.
void message_box(app_window& parent,
                 const std::string& title,
                 const std::string& message);

// Modal yes/no dialog. The callback fires when the user dismisses.
void confirm(app_window& parent,
             const std::string& title,
             const std::string& message,
             std::function<void(bool yes)> on_result);

// Modal line-edit dialog. Empty string + false when cancelled.
void input_dialog(app_window& parent,
                  const std::string& title,
                  const std::string& prompt,
                  std::function<void(bool ok, std::string value)> on_result);

// Error dialog. Shorthand for message_box with severity styling.
void error_box(app_window& parent,
               const std::string& message,
               const std::string& title = "Error");

} // namespace onyxui::simple
```

All dialog helpers internally call
`parent.host().present_modal(build_dialog_widget(...))` and wire
the result callbacks. The returned `presented_window` is owned by
the helper until the user dismisses the dialog.

---

## 7. What's under the hood

`app_window` owns four things:

1. An OS window (per-backend — SDL_Window for sdlpp, termbox
   surface for conio).
2. A renderer bound to that OS window.
3. A `ui_host<backend>` with the consumer's root mounted.
4. A per-window bookkeeping entry with the `run()` loop so the loop
   knows when all windows have closed.

`run()` is the backend-dependent main loop:

```cpp
int run() {
    while (!s_quit_requested && !s_app_windows.empty()) {
        for (auto* w : s_app_windows) {
            pump_events_for(*w);  // backend-specific event poll
            w->render_frame();    // calls w->host().render(...)
        }
        present_all();            // backend-specific flip
    }
    return s_exit_code;
}
```

The loop, event pumping, and renderer flip are backend-specific;
everything above that line is shared. Two backends mean two
implementations of the ~30-line main loop, not two implementations
of the UI story.

**Why `app_window` and the loop live in `onyxui::simple` rather
than in each backend directory:** so that the simple-shell
semantics are uniform. A tool that runs on sdlpp and on conio uses
the same API; only the shell header differs.

---

## 8. The simple / embed boundary rule

**The rule is about runtime composition, not header inclusion.**
Either tier may freely include the backend-alias header
`<onyxui/backend/...>`. Only the simple tier uses the bundle
header `<onyxui/for/...>`.

Specifically:

- An engine consumer using `ui_host<B>` (or the aliased `ui_host`)
  may include `<onyxui/backend/sdlpp.hh>` — the aliases are
  useful regardless of tier. What they must not do is call
  `onyxui::simple::run()` or instantiate `simple::app_window`,
  because those take over the main loop.
- A simple-tier consumer including `<onyxui/for/sdlpp.hh>` must
  not additionally construct a second `ui_host<B>` by hand — the
  `app_window` already owns one, and a second would fight for the
  ambient service stack on the same thread.
- The simple shell's `app_window::host()` accessor is an escape
  hatch, not an invitation to embed. It exists so a simple-shell
  consumer can call `present_modal` for custom dialogs or reach a
  specific service on the host's internal instance. It does not
  invert the boundary.

This is a composition rule, not a type-system enforcement.
Violations are noisy and obviously wrong at runtime (two competing
main loops, conflicting ambient service stacks), which is enough.

---

## 9. What's in v1 / skipped / deferred

**In v1:**

- `app_window` with `set_content`, `show`, `close`, `set_title`,
  `is_open`, `host()` escape hatch.
- `run()` and `quit()`.
- `message_box`, `confirm`, `input_dialog`, `error_box`.
- Shell headers for sdlpp and conio.
- One reference sample (`examples/simple_hello/`) demonstrating
  Hello-World.
- `widgets_demo` ported onto the simple shell, replacing its
  current `scoped_ui_context` + `ui_handle` plumbing.

**Skipped in v1 (add later if a consumer asks):**

- **Builder sugar** — `vbox().add(...)` composition DSL. Not
  required for the simple story.
- **Auto-find active `app_window` for dialogs.** v1 takes the
  parent explicitly. FLTK-style thread-local active-window can
  come later if the friction justifies it.
- **`file_open_dialog` / `file_save_dialog`.** Backend-specific
  and intrusive; defer until a consumer hits it.
- **Menu bar and status bar on `app_window`.** The existing
  `main_window<B>` is the tree widget that does this; v1's
  `app_window` is a shell *around* a root widget, not a
  pre-composed layout. If a tool wants a menu bar, it mounts a
  `main_window<backend>` as its root.

**Deferred entirely:**

- **Multi-window document apps.** A tool can create multiple
  `app_window` instances today; the loop handles them. But the
  patterns (parent/child windows, shared menus, document model)
  are not in scope.
- **Custom event loops.** If a consumer needs a custom event loop
  they should use `ui_host<B>` directly, not the simple shell.

---

## 10. Migration: `widgets_demo`

`widgets_demo` is the first non-warlords consumer and the natural
validation target.

**Before** (current): `widgets_demo/main.cc` constructs a
`scoped_ui_context` or equivalent, creates a renderer, sets up
event dispatch, wires themes, then mounts the demo UI. The
`widgets_demo.hh` header carries the render/event plumbing.

**After:**

```cpp
// examples/widgets_demo/main.cc
#include <onyxui/for/sdlpp.hh>
#include "widgets_demo.hh"

int main() {
    onyxui::simple::app_window win("OnyxUI Widgets Demo", 1024, 768);
    win.set_content(build_widgets_demo());
    win.show();
    return onyxui::simple::run();
}
```

`widgets_demo.hh` shrinks to a `build_widgets_demo()` function that
constructs the root widget tree and returns it. No service
plumbing, no render loop, no event dispatch. The spawned demo
windows (modal/modeless dialog examples, MVC demo, about dialog)
already use `presented_window` from WAR-45; under the simple
shell they route through `win.host().present(...)` or
`win.host().present_modal(...)`.

---

## 11. Dependency and sequencing

**Hard prerequisite:** `ui_host<B>` (`ONYXUI_UI_HOST_DESIGN.md`).
The simple shell is a veneer; without `ui_host`, there's nothing
to veneer.

**Implementation order:**

1. Land `ui_host<B>` (step 1 of its migration plan).
2. Port warlords' `scene_with_ui` onto `ui_host` (step 2 of its
   plan). This validates `ui_host` against the serious embedding
   consumer.
3. Add `<onyxui/for/sdlpp.hh>` and `<onyxui/for/conio.hh>` shell
   headers with the aliases.
4. Implement `onyxui::simple::app_window` for sdlpp.
5. Implement `onyxui::simple::run()` / `quit()` for sdlpp.
6. Implement dialog helpers (`message_box`, etc.) for sdlpp.
7. Port `widgets_demo` onto the simple shell.
8. Implement conio variants of steps 4–6. Verify `widgets_demo`
   runs under conio too (re-include the conio shell header,
   recompile).

Steps 3–7 are the sdlpp path; step 8 proves the abstraction holds
across backends.

---

## 12. Open questions

These don't block prototyping but should be answered as
implementation surfaces them:

- **Single-window vs multi-window for `run()` exit semantics.**
  "Run until the last `app_window` closes" is FLTK-style. "Run
  until the primary `app_window` closes" is Windows-style. v1
  leans FLTK. Confirm with a use case.

- **Signal-handler integration for Ctrl-C.** `run()` should
  probably treat SIGINT as a quit request. Per-backend detail.

- **Non-decorated (borderless / fullscreen) `app_window`.** Tools
  like splash screens may want this. v1 has a decorated window.
  Add flags when a consumer needs them.

- **Simple-shell tests.** Unit-test the app_window and run loop —
  likely requires a headless backend or a test harness that
  doesn't spin a real window. Defer until implementation.

- **Dialog helper callback lifetime.** If `message_box(parent,
  ...)` is fire-and-forget, what owns the presented_window until
  the user dismisses? Implementation detail — likely a vector of
  in-flight presentations on `app_window`. Document when
  implemented.

- **`app_window::host()` is public — is that a mistake?** v1
  exposes it as an escape hatch. If that turns out to be a leak
  (consumers reach for it unnecessarily), gate it behind a
  `friend` or a specific accessor later. Keep it visible in v1 to
  unblock the warlords-style "I need present_modal" case.

---

## 13. Next step

This doc is the proposal. Before implementing:

1. Review and approve the per-backend shell header mechanism in §5.
2. Review and approve the simple-shell API in §6.
3. Confirm `widgets_demo` is the correct first validation consumer.
4. Decide whether step 3–7 (sdlpp path) lands in one PR or
   multiple.

Recommended first PR: steps 3 + 4 + 5 together, producing a working
`simple::app_window::show()` that renders a mounted widget and
returns from `run()` when closed. Dialog helpers (step 6) and the
`widgets_demo` port (step 7) can be follow-up PRs. That sequencing
proves the core shell mechanics before accreting features.
