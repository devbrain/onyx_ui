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
- teardown order careful enough that overlays don't outlive the
  context they reference

(Error surfacing — getting an async failure in front of the user —
is explicitly **not** absorbed by `ui_host`; see §5.)

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

    // Unmount returns the root for inspection or reuse. Subsequent
    // render() / handle_event() calls render an empty root
    // (the host's layer stack is unaffected — see §7.1).
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

    // --- Escape hatches (rarely needed) ---

    [[nodiscard]] layer_manager<B>&   layers()    noexcept;
    [[nodiscard]] theme_registry<B>&  themes()    noexcept;
    [[nodiscard]] focus_manager<B>&   focus()     noexcept;
    [[nodiscard]] hotkey_manager<B>&  hotkeys()   noexcept;
    [[nodiscard]] input_manager<B>&   input()     noexcept;
    [[nodiscard]] const backend_metrics<B>& metrics() const noexcept;
};
```

## 5. Error handling is not a `ui_host` responsibility

Earlier drafts of this doc placed an error channel on `ui_host`
(a `report_error` / `take_error` queue). That was wrong — it
broke warlords' recovery model.

In warlords today, errors enqueue in app-global state:

- `src/bane/main.cc` — the `SDL_AppIterate` / `SDL_AppEvent`
  safety-net catches wrap exceptions and enqueue to
  `bane::services::report_error`,
- `src/bane/kernel/scenes_manager.cc` — scene-push rollback
  (failure during `on_enter`) enqueues the error before popping
  the broken scene,
- `src/bane/scenes/init_scene.cc` — the surviving scene drains
  the queue next frame and shows an error dialog.

The protocol's critical invariant is that **the error outlives
the failing scene**. If the queue lived on that scene's `ui_host`,
the push/on_enter/frame failure → scene pop → queue destruction
sequence would destroy the error before any replacement scene
could display it.

The UI framework is therefore the wrong owner for the error
channel. It stays an app-level concern:

- **warlords** keeps `bane::services::report_error` /
  `take_error` as an app-global queue, unchanged by the `ui_host`
  migration.
- **The simple shell** (see `ONYXUI_SIMPLE_SHELL_DESIGN.md`) owns
  its own error surface on `onyxui::simple::app_window` or in
  `onyxui::simple::` globals — the queue lives in the
  `simple::run()` scope, which outlasts individual
  mount/unmount cycles.
- **`ui_host`** provides no error channel. A consumer that wants
  one wires its own app-level queue and routes errors through it,
  independent of which host instance is current.

`ui_host` still surfaces library-originated conditions through
signals (e.g., `window::closed`) as it does today. What it does
not do is become the app-global sink for cross-transition
failures — that's the consumer's responsibility.

---

## 6. What gets replaced

No backward compatibility required.

| Existing type/API                              | Fate                                                                       |
|------------------------------------------------|----------------------------------------------------------------------------|
| `scoped_ui_context<B>`                         | Becomes an internal implementation detail of `ui_host<B>`. The scope-stack push/pop still happens, just inside `render()` / `handle_event()`. Removed from public headers. |
| `ui_handle<B>`                                 | Deleted. Its role (bind renderer to root, drive render/event) is absorbed by `ui_host::render()` / `handle_event()`. Renderer-per-call replaces renderer-ownership. |
| `scene_with_ui` teardown-order comment         | Deleted. The ownership story becomes explicit at the type level. |
| `warlords::services::report_error / take_error`| **Unchanged.** Stays app-global in `bane::services::`. Not absorbed by `ui_host`; see §5 for rationale. |

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

### 7.1 `unmount()` and live overlays

`unmount()` only affects the root — the layer stack and any live
`presented_window<B>` handles are untouched. Specifically:

- `unmount()` returns the root widget. The host's layer manager
  and every layer currently registered through `present()` or
  `present_modal()` remain alive.
- Subsequent `render()` calls draw the layer stack over an empty
  background until `mount()` installs a new root.
- Subsequent `handle_event()` calls continue to dispatch to
  overlays; routing to the unmounted root no-ops.
- `mount()` installs a replacement root without disturbing live
  overlays — they render over the new root exactly as they
  rendered over the old one.

This keeps ownership consistent with the rest of the design:
overlays are owned by the consumer-held `presented_window<B>`
handles, not by the host. `unmount()` doesn't get to dismiss
someone else's owned state. A consumer that wants a clean
transition drops presenter handles explicitly before calling
`unmount()`.

Ghost-overlay scenario (render over nothing) is deliberately
reachable: that's what the consumer asked for by calling
`unmount()` without dropping their presenters. It's a valid
transient state during mount-swap and doesn't UAF — the layer
manager and all widgets remain valid.

**Destruction:** when `ui_host` itself dies, the layer manager
dies with it; the existing `window::m_layer_owner_conn` mechanism
(shipped in WAR-45) handles the out-of-order case where a
`presented_window` outlives the host. The window destructor
degrades to a visibility-flag update and doesn't UAF.

### 7.2 What owns what

**The host owns:**
- service stack (themes, focus, hotkeys, input, layers, metrics),
- the mounted root widget tree,
- internal state needed for overlay layer registration.

It does **not** own the error queue — see §5.

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
its own services and its own layer stack. Consumers who want
cross-host state pass it explicitly. (The app-level error queue,
per §5, is not a host concern.)

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
   §4 describes. Add focused unit tests: construction, mount /
   unmount (including unmount-with-live-overlays per §7.1),
   render, handle_event, present / present_modal, destruction
   ordering. No error-channel tests — error surfacing is not a
   host responsibility (§5).

2. **Port `scene_with_ui` onto `ui_host`.** Delete the
   teardown-order comment. `host_.render()` and
   `host_.handle_event()` replace the direct `scoped_ui_context` /
   `ui_handle` plumbing. `bane::services::report_error` /
   `take_error` **stay where they are** — they're an app-level
   channel that must outlive individual host instances (§5).

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
  414 tests still pass. `bane::services::report_error` /
  `take_error` remain in place and keep surfacing cross-scene
  errors (§5).
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

1. Review and approve the API in §4 and the unmount-vs-overlays
   contract in §7.1.
2. Confirm the §5 decision that error surfacing stays app-level,
   not absorbed by `ui_host`.
3. Lock the migration plan in §9 — specifically, whether step 2
   (warlords port) lands in the same PR as step 1 or separately.
4. Decide whether to start prototyping.

Recommended first PR: step 1 + step 2 together, so the `ui_host`
design is validated against the real embedding consumer from the
start. Without the warlords port in the same PR, step 1 is
speculative; with it, the design either works or obvious gaps
surface immediately.
