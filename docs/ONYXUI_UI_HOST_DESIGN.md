# OnyxUI `ui_host<B>` Design

> Status: proposal (2026-04-19)
> Companion to: [ONYXUI_RUNTIME_DIRECTION.md](./ONYXUI_RUNTIME_DIRECTION.md) (adopted)
> Scope: consolidate embedding-side host plumbing into one type
> Non-scope: app-shell layer, renderer ownership, backward compatibility

---

## 1. Why this exists

OnyxUI's core is in good shape; embedding it into a custom host is
not. To mount a UI today, a consumer has to understand and assemble:

- `scoped_ui_context<B>` (with `backend_metrics<B>`)
- `theme_registry<B>` (via ctx)
- `hotkey_manager<B>` (via ctx)
- `focus_manager<B>` (via ctx)
- `input_manager<B>` (via ctx)
- `layer_manager<B>` (via ctx)
- `ui_handle<B>` (wraps root widget + binds renderer)

…and then write:

- event dispatch from its own source into `ui_handle`
- render calls at the right frame tick
- overlay presentation through `layer_manager` + `presented_window`
- an error channel for async subsystems that need user-visible
  failure surfacing (warlords built this locally as
  `services::report_error`)
- teardown order careful enough that overlays don't outlive the
  context they reference

`scene_with_ui` in warlords is the de-facto embedding contract, but
only by accident — nobody outside warlords knows it exists. The
adopted runtime direction doc §0 lists
"`scene_with_ui` teardown-comment cleanup" as an outstanding item;
that comment exists because the ownership story is implicit.

**The fix is not a guide. It is a type that makes the ownership
story explicit in code.**

The user-visible payoff: "mount this UI and run it in my loop" goes
from seven types + two hand-written protocols to **one object
construction + two per-frame entry points**.

---

## 2. Goals

- One type to construct for "mount and run UI in my host".
- Explicit render / event / presentation entry points at the level
  engines naturally think in.
- Hide the service-stack plumbing and ambient-lookup machinery
  behind the type.
- Provide escape hatches for power users who legitimately need
  individual services (theme, layers, focus, etc.) without forcing
  them through a side door.
- Replace `scoped_ui_context` and `ui_handle` outright — this is not
  a compatibility shim.

## 3. Non-goals

- **Not a replacement for `layer_manager`, `theme_registry`, etc.**
  Those stay public and accessible as escape hatches.
- **Not an app-shell.** Track C of the broader plan builds on top;
  app-shell is "ui_host + a render loop".
- **Not a renderer owner.** Rendering is a per-frame consumer call
  with a consumer-owned renderer.
- **Not a backward-compatibility shim.** Prior host types get
  replaced, not preserved.

---

## 4. API sketch

```cpp
template<UIBackend B>
class ui_host {
public:
    // --- Construction ---
    //
    // Metrics default to backend-appropriate 1:1 values suitable for
    // non-HiDPI scenarios; consumers with custom DPI pass their own.
    explicit ui_host(backend_metrics<B> metrics = {});
    ~ui_host();

    // Not copyable or moveable. The internal services hold raw
    // back-pointers; moving would invalidate them.
    ui_host(const ui_host&) = delete;
    ui_host& operator=(const ui_host&) = delete;
    ui_host(ui_host&&) = delete;
    ui_host& operator=(ui_host&&) = delete;

    // --- Root widget tree ---

    void mount(std::unique_ptr<ui_element<B>> root);
    [[nodiscard]] ui_element<B>* root() noexcept;
    [[nodiscard]] const ui_element<B>* root() const noexcept;

    // Unmount returns the root for inspection or reuse; subsequent
    // render() calls no-op until another mount().
    [[nodiscard]] std::unique_ptr<ui_element<B>> unmount();

    // --- Per-frame entry points ---

    // Consumer calls render() every frame with its own renderer and
    // the viewport rect the UI should fill. Host pushes its scoped
    // service state around measure/arrange/paint.
    void render(typename B::renderer_type& renderer,
                logical_rect viewport);

    // Consumer calls handle_event() for every event from its own
    // source. Returns true if the event was consumed by the UI.
    // Host pushes its service state for the dispatch duration.
    [[nodiscard]] bool handle_event(const typename B::event_type& ev);

    // --- Overlay presentation ---
    //
    // No explicit layer_manager argument — the host owns its own
    // layers. The returned presented_window is the presentation;
    // dropping it dismisses the overlay.

    [[nodiscard]] presented_window<B> present(
        std::unique_ptr<window<B>> win);

    [[nodiscard]] presented_window<B> present_modal(
        std::unique_ptr<window<B>> win,
        dialog_position pos = dialog_position::center);

    // --- Error channel ---

    void report_error(error_info info);
    [[nodiscard]] std::optional<error_info> take_error();

    // --- Escape hatches (rarely needed) ---

    [[nodiscard]] layer_manager<B>&   layers()    noexcept;
    [[nodiscard]] theme_registry<B>&  themes()    noexcept;
    [[nodiscard]] focus_manager<B>&   focus()     noexcept;
    [[nodiscard]] hotkey_manager<B>&  hotkeys()   noexcept;
    [[nodiscard]] input_manager<B>&   input()     noexcept;
    [[nodiscard]] const backend_metrics<B>& metrics() const noexcept;
};
```

## 5. `error_info`

A richer error than a bare string. Async subsystems need to
communicate severity (is this fatal? recoverable?) and origin (which
subsystem?) so host UI can render it correctly (red modal dialog vs.
status-bar note vs. silent log).

```cpp
enum class error_severity : std::uint8_t {
    info,       // informational; non-blocking
    warning,    // user should know, can continue
    error,      // operation failed; user must acknowledge
    fatal,      // host compromised; consider bailing
};

struct error_info {
    error_severity severity = error_severity::error;
    std::string    message;    // user-facing text
    std::string    source;     // subsystem id, e.g. "net", "import"
    std::chrono::steady_clock::time_point when =
        std::chrono::steady_clock::now();
};
```

**Queue semantics:** FIFO. `take_error()` pops the oldest; consumer
drains per frame. Warlords' current `init_scene::update_physics` loop
already has this shape — the `std::string` it reads becomes
`error_info{severity=error, source="…", message=…}`.

**Bound:** not set at the API level in v1; implementation caps with
drop-oldest at an internal limit (e.g. 64). If a consumer needs a
bigger or different bound, add a constructor parameter later.

---

## 6. What gets replaced

No backward compatibility required.

| Existing type/API                              | Fate                                                                       |
|------------------------------------------------|----------------------------------------------------------------------------|
| `scoped_ui_context<B>`                         | Becomes an internal implementation detail of `ui_host<B>`. The scope-stack push/pop still happens, just inside `render()` / `handle_event()`. Removed from public headers. |
| `ui_handle<B>`                                 | Deleted. Its role (bind renderer to root, drive render/event) is absorbed by `ui_host::render()` / `handle_event()`. Renderer-per-call replaces renderer-ownership. |
| `warlords::services::report_error / take_error`| Move into `ui_host`. The `bane/kernel/services.{hh,cc}` namespace shrinks to the game-service accessors it actually belongs to. |
| `scene_with_ui` teardown-order comment         | Deleted. The ownership story becomes explicit at the type level. |

After migration, `scene_with_ui` collapses to roughly:

```cpp
class scene_with_ui : public base_scene {
protected:
    ui_host<warlords_backend> host_;

    void render_frame(typename warlords_backend::renderer_type& r,
                      logical_rect viewport) {
        host_.render(r, viewport);
    }

    bool forward_event(const typename warlords_backend::event_type& ev) {
        return host_.handle_event(ev);
    }
};
```

The dialog-migration shape from WAR-45 is unchanged: `init_scene`
still holds `std::unique_ptr<selector_dialog> m_selector;` etc. Only
the presentation call changes — from
`m_selector = std::make_unique<selector_dialog>(layers(), …)`
to the equivalent routed through `host_.present(…)` (builder object
becomes thinner, since the dialog class no longer needs a
layer_manager reference).

---

## 7. Ownership and lifetime

**The host owns:**
- service stack (themes, focus, hotkeys, input, layers, metrics),
- the mounted root widget tree,
- the error queue,
- internal state needed for overlay layer registration.

**The consumer owns:**
- the renderer (passed per `render()` call),
- the event source,
- the render / event loop,
- any `presented_window<B>` handles it stores.

**The host does NOT own:**
- the renderer,
- the event loop,
- the game/app state,
- widget trees mounted as overlays (those live in the presenter
  until it's dropped).

**Destruction protocol:** when `ui_host` dies, its service stack
dies with it. Any still-alive `presented_window<B>` observes its
owning `layer_manager` being destroyed via the existing
`m_layer_owner_conn` scoped_connection (shipped in WAR-45), so the
window's destructor degrades to a visibility-flag update and does
not UAF. This protocol already works; `ui_host` just makes it the
only lifetime story consumers need to know about.

**Scope-stack ambient services:** the existing
`ui_services<B>::layers() / themes() / input()` pipeline still
exists for internal and render-pipeline use. `ui_host::render()`
and `handle_event()` push/pop the scope-stack internally. Between
calls, the host is "dormant" — no services are ambient. This is
consistent with the rule in `PRESENTATION_CONVENTIONS.md`: ambient
lookups are for render / event / layout, not for consumer business
logic.

---

## 8. Multiple hosts

Default: one host per consumer.

**Supported:** multiple hosts in the same process. Reasonable use
cases include a separate HUD host and menu host in the same app, an
embedded-tests harness that spins up throwaway hosts, or
picture-in-picture UI.

**Sharing:** nothing is shared between hosts by default. Each has
its own services, its own layer stack, its own error queue.
Consumers who want cross-host state pass it explicitly.

**Scope-stack nesting with multiple hosts:** two hosts rendering
concurrently on different threads would be a consumer bug — the
ambient scope stack is a thread-local singleton. Two hosts on the
same thread that render sequentially (host A's `render()` completes
before host B's begins) compose fine; each pushes and pops around
its own call.

---

## 9. Migration plan

Step-by-step, each step independently mergeable:

1. **Add `ui_host<B>` alongside existing types.** Internally built
   from `scoped_ui_context` + `ui_handle`. Public surface is what
   §4 describes. Add focused unit tests (construction, mount,
   render, handle_event, present, error channel, destruction).

2. **Port `scene_with_ui` onto `ui_host`.** Delete the
   teardown-order comment. `host_.render()` and
   `host_.handle_event()` replace the direct `scoped_ui_context` /
   `ui_handle` plumbing. warlords `services::report_error /
   take_error` moves into `ui_host` (bane's `services` namespace
   loses the error forwarding).

3. **Port `widgets_demo` onto `ui_host`.** This is a prerequisite
   for collapsing the app-shell (Track C) to meaningful size;
   once `ui_host` exists, the standalone-tool story is "construct
   a host, mount a widget, run a loop" — ~30 lines of backend-
   specific glue.

4. **Remove external callers of `scoped_ui_context` / `ui_handle`.**
   Audit tests, backend integration code, anywhere else. Replace
   each with `ui_host`.

5. **Delete `scoped_ui_context` / `ui_handle` from public headers.**
   Internal moves into a `detail::` namespace or implementation
   files. `ui_services<B>::*()` stays public (pipeline internals
   depend on it).

Success criteria at each step:
- Step 1: `ui_host` has tests; no consumer uses it yet.
- Step 2: `scene_with_ui` teardown-order comment is gone; warlords
  414 tests still pass; bane's `services` namespace is smaller.
- Step 3: `widgets_demo` no longer constructs `scoped_ui_context`
  or `ui_handle` directly.
- Step 4: zero public call sites for the old types.
- Step 5: a `git grep scoped_ui_context` from outside onyx_ui
  returns nothing.

---

## 10. Open questions

These don't block prototyping but should be answered as
implementation surfaces them:

- **Metrics mutability.** `backend_metrics<B>` is passed at
  construction — but HiDPI scale can change at runtime (monitor
  switch, DPI-override). `ui_host` should probably accept a
  `set_metrics(metrics)` that invalidates layout. Defer until a
  consumer hits this.

- **Initial focus on mount.** When `mount()` is called, does the
  first focusable widget get focus automatically, or does the
  consumer call `host.focus().set_focus(...)` explicitly? Current
  `ui_handle` defers to the consumer. Recommend keeping that
  (explicit focus management), document it.

- **Error queue cap.** Implementation caps at ~64 with drop-oldest.
  Is that configurable? Needed? Defer unless a consumer asks.

- **Per-host theme override across multiple hosts.** If host A sets
  theme "dark" and host B sets theme "light", does the scope-stack
  push/pop correctly during their render calls so each sees its own
  current theme? Add a test when building; expect it to work based
  on the existing theme registry design.

- **Overlay stacking across hosts.** Can an overlay presented by
  host A appear above host B's content? Default: no — each host
  has its own layer stack. Cross-host overlay stacking would require
  a shared compositor. Not in scope for v1.

- **Render vs update separation.** Does `ui_host` need an explicit
  `update(delta)` for animation clock advancement, separate from
  `render()`? OnyxUI animations today are driven by render-time
  state; `render()` is enough. Revisit if an animation subsystem
  needs non-render ticks.

---

## 11. Next step

This doc is the proposal. Before implementing:

1. Review and approve the API in §4 and the `error_info` shape in §5.
2. Lock the migration plan in §9 — specifically, whether step 2
   (warlords port) lands in the same PR as step 1 or separately.
3. Decide whether to start prototyping.

Recommended first PR: step 1 + step 2 together, so the `ui_host`
design is validated against the real embedding consumer from the
start. Without the warlords port in the same PR, step 1 is
speculative; with it, the design either works or obvious gaps
surface immediately.
