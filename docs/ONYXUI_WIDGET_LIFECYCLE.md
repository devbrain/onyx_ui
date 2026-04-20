# OnyxUI Widget Lifecycle (WAR-64)

> Status: adopted (2026-04-20)
> Linear: [WAR-64](https://linear.app/warlords/issue/WAR-64/widget-lifecycle-move-ambient-service-lookups-out-of-constructors)
> Companion to: [ONYXUI_UI_HOST_DESIGN.md](./ONYXUI_UI_HOST_DESIGN.md)
> Scope: widget construction / attachment / teardown contract
> Non-scope: layout algorithm changes, renderer API, public widget surface
>
> **Implementation status:** Steps 1–4 landed (commits afa8fd6,
> 5644c7f, 672fcab, c09fb5d, 20b34bc). `with_scope` deleted.
> `push_scope` / `scope_token` kept as the RAII primitive for
> test fixtures that drive widget measure/arrange outside of
> `host.render()`. Both `menu_bar` and `text_view` ported —
> constructor-time ambient-service lookups are now banned and
> verified by WAR-57's former crash path no longer reproducing.

---

## 1. TL;DR

Two widgets — `menu_bar<B>` and `text_view<B>` — consult ambient
services (`ui_services<B>::themes()` / `::hotkeys()`) from their
**constructors**. Every consumer that builds a widget tree outside a
`ui_host` scope has to wrap the construction in
`host.with_scope([&]{ ... })` or `host.push_scope()` to keep those
lookups alive. That's why WAR-57 needed `with_scope` in
`build_widgets_demo`, and why `ui_context_fixture` in the test
harness holds a `scope_token` for every test's lifetime.

This document proposes moving the two problem call sites onto a
post-attach hook so widget construction becomes pure. Once that
lands, `ui_host::with_scope` and `push_scope` can shrink to
test-only helpers (or go away entirely).

**Scope is smaller than the original WAR-64 description implied.**
The audit (§3) found only two constructor-time service lookups
across all 26 widget files, not a tree-wide problem. The fix is a
focused two-widget refactor plus a small new hook, not an
architectural overhaul.

---

## 2. The problem in one example

From `include/onyxui/widgets/text_view.hh`:

```cpp
// Line 97 — constructor calls create_scroll_view()
text_view(...) {
    ...
    create_scroll_view(...);
}

// Line 479-480 — create_scroll_view() reaches ambient
auto* theme = ui_services<Backend>::themes()->get_current_theme();
content_container_ptr->set_padding(
    logical_thickness(logical_unit(theme->text_view.padding)));
```

`ui_services<Backend>::themes()` returns `nullptr` when no host
scope is active. The constructor dereferences it. The result is a
segfault — the exact crash WAR-57's `sdlpp_widgets_demo`
investigation hit, which forced the introduction of
`ui_host::with_scope` as a workaround.

The same pattern exists in `menu_bar.hh:151` →
`initialize_hotkeys()` → line 441 calls
`ui_services<Backend>::hotkeys()` to register the F10/F9 activation
handlers during construction.

Neither is an actual hard requirement. `text_view` needs the theme
for initial padding — something that applies at first render
anyway, because themes can change at runtime. `menu_bar` needs the
hotkey manager for a one-shot registration — which only matters
once the menu bar is actually part of a live tree.

---

## 3. Audit findings

Spanning `include/onyxui/widgets/` (26 files referencing
`ui_services<Backend>::…`):

| Widget | Constructor service lookup? | Deferred (render/event) | Notes |
|--------|:-:|:-:|---|
| `menu_bar` | **YES** (hotkeys via `initialize_hotkeys()`, l.151) | — | One-shot registration |
| `text_view` | **YES** (themes via `create_scroll_view()`, l.97) | — | Needed for initial padding |
| `button` | no | yes (l.175, 198, 262) | Correct pattern |
| `checkbox`, `radio_button`, `slider`, `progress_bar` | no | yes | Correct |
| `line_edit`, `list_box`, `combo_box` | no | yes | Correct |
| `scroll_view`, `scrollbar`, `tab_widget`, `menu`, … | no | yes | Correct |
| *(… 20+ more …)* | no | yes | All deferred |

**Conclusion:** 24 / 26 widgets already follow the correct pattern
(lookups in `render()` / `handle_event()` / `measure()`). The
refactor only needs to fix two files.

Other findings worth documenting:

- `ui_host::mount(root)` does **not** push a scope — it just stores
  the root.
- `ui_element::add_child` / `emplace_child` just append to the
  child vector and invalidate measure. No attachment callback
  exists today.
- No `on_attached` / `on_inserted` / `initialize` hook exists on
  `ui_element` or anywhere above it.
- `window_presets`, `main_window`, `tab_widget`, `menu_bar_item`
  build their child trees post-construction (via
  `set_central_widget()` / `add_tab()` / `add_menu()`) — so the
  tree can be grown lazily without service access.
- All existing tests use `ui_context_fixture`, which holds a
  `scope_token` for the test's lifetime. Widget construction in
  every test is already inside a scope — so the refactor won't
  break the test harness. Once `on_attached` works, we can
  eventually drop the fixture's `scope_token` member entirely.

---

## 4. Design: the lifecycle contract

### 4.1 Phases

A widget's life has four observable phases:

```
┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
│  constructed │──▶│  attached   │──▶│  measured  │──▶│  rendered   │
└─────────────┘   └─────────────┘   └─────────────┘   └─────────────┘
   (ctor runs)     (on_attached)    (measure/arrange)    (render)
```

Each phase has a strict contract about what ambient state is
available.

### 4.2 `constructed` — pure

- Constructor sets member defaults, builds child structure that
  does NOT depend on backend services, wires up signal/slot
  connections that don't require a renderer.
- **MAY NOT call `ui_services<B>::…`**. No theme, hotkey, layer,
  input, or window-manager access.
- **MAY emplace children.** Child constructors run in the same
  phase — they're pure too.
- **MAY refer to `Backend::color_type` / geometric types / enums.**
  These are compile-time facts, not ambient services.

### 4.3 `attached` — services available, tree known

- Fires when the widget has been inserted into a live widget tree
  that `ui_host::mount()` has claimed. A freshly-constructed
  subtree becomes attached all at once when its root is mounted.
- Widgets override `on_attached(ui_host<Backend>&)` to register
  hotkeys, resolve themes, snap initial padding, etc.
- **MAY call `ui_services<B>::…`** — the hook fires inside a scope
  the host pushed.
- Ordering: parent is attached, then children (pre-order). This
  lets parent-driven registration (e.g. a menu bar grabbing F10
  hotkeys) complete before children try to claim their own
  bindings.
- **Runs once per `mount()` call** — on `unmount()` + re-mount,
  `on_attached` fires again. Widgets that stored theme-derived
  values should be prepared to refresh them.

### 4.4 `measured` / `arranged` — geometry known

- Existing behavior. No change.
- `measure()` and `arrange()` may consult services (e.g. to fetch
  current metrics). They already do, and that's fine.

### 4.5 `rendered` — renderer available

- Existing behavior. No change.
- `do_render()` is the canonical place to resolve theme state
  that's allowed to change frame-to-frame. Widgets that stored a
  theme pointer in `on_attached` should either re-resolve in
  `do_render`, or subscribe to `theme_registry::theme_changed` and
  invalidate on change.

### 4.6 Detachment

- Symmetric hook: `on_detached(ui_host<Backend>&)` fires before a
  widget leaves the tree (via `unmount()`, child removal, or host
  destruction).
- Use case: unregister hotkeys, cancel async work, drop cached
  theme pointers.
- Optional override — most widgets don't need it.

---

## 5. New API

```cpp
// include/onyxui/core/element.hh (additions)
template<UIBackend Backend>
class ui_element {
public:
    // ... existing ...

    /// Called exactly once when this element has been added to a
    /// widget tree that is mounted on a `ui_host<Backend>`. The
    /// host's context is pushed as the ambient scope, so
    /// `ui_services<Backend>::...` lookups are valid.
    ///
    /// Runs in pre-order: parent first, then children. Override
    /// to resolve ambient state (themes, register hotkeys, etc.).
    ///
    /// Default is a no-op. Widgets that do not need any ambient
    /// setup should not override this.
    virtual void on_attached(ui_host<Backend>& host) { (void)host; }

    /// Called exactly once when this element is about to leave a
    /// tree that was mounted on `host`. Symmetric with on_attached;
    /// use it to unregister hotkeys, drop cached theme pointers,
    /// etc. Default is a no-op.
    virtual void on_detached(ui_host<Backend>& host) { (void)host; }

protected:
    // ... existing ...
};
```

```cpp
// include/onyxui/ui_host.hh (mount()/unmount() become active)
template<UIBackend Backend>
class ui_host {
public:
    void mount(std::unique_ptr<widget_type> root) {
        // If we had a prior root, detach it first (outside the new
        // scope to avoid the parent's on_detached seeing its
        // replacement).
        if (m_root) {
            scope guard(m_ctx);
            dispatch_detached(*m_root);
        }

        m_root = std::move(root);

        if (m_root) {
            scope guard(m_ctx);
            dispatch_attached(*m_root);
        }
    }

    [[nodiscard]] std::unique_ptr<widget_type> unmount() {
        if (m_root) {
            scope guard(m_ctx);
            dispatch_detached(*m_root);
        }
        return std::move(m_root);
    }

private:
    void dispatch_attached(widget_type& w) {
        w.on_attached(*this);
        for (auto& child : w.children()) {
            if (child) dispatch_attached(*child);
        }
    }
    void dispatch_detached(widget_type& w) {
        // Reverse order — children first, then parent.
        for (auto& child : w.children()) {
            if (child) dispatch_detached(*child);
        }
        w.on_detached(*this);
    }
};
```

### 5.1 Late-added / removed children — v1 limitation

**v1 does NOT fire `on_attached` / `on_detached` on the
late-mutation paths** (`ui_element::add_child` /
`clear_children` / `remove_child` invoked on a subtree that is
already part of a mounted tree). The reason is mechanical rather
than design-driven:

- `ui_services.hh` transitively depends on `ui_element` (via
  `hotkey_manager`, `input_manager`, …).
- So `element.hh` cannot include `ui_services.hh`, which means
  `ui_element::add_child` has no way to push the host's ambient
  scope around a hook dispatch.
- Firing the hook without a pushed scope is *worse* than not
  firing it — `ui_services<B>::themes()` / `::hotkeys()` return
  nullptr, silently breaking any widget that reads them.

Until a cycle break lets `element.hh` see `ui_services.hh`,
late-mutation paths clear parent/host pointers without
dispatching. Consumers that mutate a mounted tree and need
lifecycle hooks on the new shape should **re-mount**:

```cpp
// Want late-attach semantics? Unmount + re-mount.
auto root = host.unmount();
root->add_child(std::move(new_subtree));
host.mount(std::move(root));   // fires on_attached across the whole tree
```

In practice widgets_demo, the simple-shell samples, and
warlords build their trees entirely before `host.mount()`, so
this limitation never bites them. The reviewer's original
concern is resolved: the mount/unmount paths are the ONLY ones
that fire hooks, and they're the ones that DO push scope.

### 5.2 Pre-construction services — not in scope

Widgets occasionally need backend-specific types (e.g. `color_type`)
at construction time. Those are compile-time traits and are already
fine — nothing in this design restricts them. Only ambient-service
*lookups* are banned from ctors, and only because those lookups
return `nullptr` without a scope.

---

## 6. Per-widget migration

### 6.1 `menu_bar<B>`

Current (`menu_bar.hh:129-152`):

```cpp
menu_bar() {
    // ... layout setup ...
    initialize_hotkeys();   // <-- calls ui_services<B>::hotkeys()
}
```

After:

```cpp
menu_bar() {
    // ... layout setup only; no service access ...
}

void on_attached(ui_host<Backend>& host) override {
    ui_element<Backend>::on_attached(host);
    initialize_hotkeys();   // now safe — scope is active
}

void on_detached(ui_host<Backend>& host) override {
    // Unregister our hotkeys so a re-mount with a different
    // menu_bar doesn't leak handlers onto the new tree.
    teardown_hotkeys();
    ui_element<Backend>::on_detached(host);
}
```

`teardown_hotkeys` is new code — paired with the existing
`initialize_hotkeys`. It's small: the menu_bar already remembers
the action handles it registered, so unregistration is just
erasing them from `host.hotkeys()`.

### 6.2 `text_view<B>`

Current (`text_view.hh:97`):

```cpp
text_view(...) : ... {
    create_scroll_view(...);   // hits ui_services<B>::themes()
}

// line 479:
void create_scroll_view(...) {
    auto* theme = ui_services<Backend>::themes()->get_current_theme();
    content_container_ptr->set_padding(
        logical_thickness(logical_unit(theme->text_view.padding)));
    // ... more widget setup ...
}
```

Two approaches, pick one per concern:

- **Resolve at first render.** Defer the padding pull to
  `do_render()` / `do_measure()` (where `text_view.hh` already
  queries themes at line 426). The content container is
  constructed with a default padding; render/measure refresh it
  from the current theme each time. This is the lowest-risk path
  since the frame-by-frame re-resolve model already exists.

- **Resolve on attach.** Move the theme pull into
  `on_attached` — snap the padding once, cache it, update it
  again on theme change (the registry emits
  `theme_changed`; `text_view` already holds a connection for
  other purposes). Cleaner if the padding is expensive to
  compute, but adds a subscription.

Recommendation: **attach-time** (option B). It matches the
"snap one-off state at tree birth" shape that `menu_bar` also
uses, keeps `do_render` lightweight, and aligns with the signal
subscription `text_view` already has. Option A stays available
as a fallback if attach-time turns out to be too eager.

### 6.3 Other widgets

**No changes needed.** The audit confirmed every other widget
already resolves services in `do_render()` / `handle_event()` /
`measure()`, which will continue to push-scope internally via
`ui_host`'s per-call guard.

---

## 7. Test impact

### 7.1 Fixture simplification

`unittest/utils/test_helpers.hh::ui_context_fixture` currently
holds:

```cpp
ui_host<Backend> ctx;
typename ui_host<Backend>::scope_token m_scope;
```

because test bodies construct widgets outside `render()` and need
ambient themes to work.

After the refactor:

- Widgets no longer need ambient scope at construction → the
  `scope_token` member is only needed for tests that call
  `ui_services<B>::…` directly (not via a widget). We keep it as a
  convenience for those tests but stop relying on it for widget
  construction.
- `mount()` now pushes scope internally for `on_attached` — tests
  that mount trees get the right semantics for free.

### 7.2 Regression surface

100% of existing tests construct widgets inside the fixture's
scope. After the refactor they still do — just via a stricter
discipline. No test should need source changes.

**Visual-equivalence verification** is the main risk. For the two
ported widgets, add a before/after render-to-canvas comparison to
confirm the deferred theme resolution produces the same initial
frame. If visuals diverge, fall back to option A (render-time
resolve) for that widget.

---

## 8. Follow-up cleanup (enabled by this refactor)

- **Delete `ui_host::with_scope(F&&)`.** Factory functions no
  longer need to wrap widget construction in a scope guard —
  construction is pure, and attachment pushes scope for them.
- **Demote `ui_host::push_scope()` to test-only.** A few tests
  exercise `ui_services<B>::…` directly and still need explicit
  ambient setup; everyone else routes through mounted trees.
- **Simplify `widgets_demo` factory.** WAR-57's
  `build_widgets_demo` wraps construction in `host.with_scope(...)`.
  Post-refactor the wrapper goes and the factory is one line:
  `return std::make_unique<widgets_demo>(parent);`
- **Simplify `ui_context_fixture`.** Drop the `scope_token` member
  if remaining tests don't need it (most exercise widget behavior,
  not ambient services).

---

## 9. Implementation plan

Land in the following order — each step is independently
committable, builds clean, and keeps tests green.

### Step 1 — Add the hook infrastructure (no behavior change)

- Add `on_attached(ui_host&) = default-no-op` and
  `on_detached(ui_host&) = default-no-op` to `ui_element`.
- Add `ui_host::dispatch_attached` / `dispatch_detached` (private).
- Wire `ui_host::mount()` / `unmount()` to dispatch.
- Wire `ui_element::add_child` to propagate attach when already in
  a mounted tree.
- Tests: add one case that verifies `on_attached` fires exactly
  once, in pre-order, inside a scope.

At this point nothing calls the hook. Builds are a no-op change.

### Step 2 — Port `menu_bar`

- Move `initialize_hotkeys()` call from ctor to `on_attached`.
- Add `teardown_hotkeys()` + wire to `on_detached`.
- Keep `widgets_demo` / tests passing.

### Step 3 — Port `text_view`

- Move the theme-padding resolve from `create_scroll_view()` to
  `on_attached`.
- If `theme_changed` refresh wasn't already wired, subscribe in
  `on_attached`, unsubscribe in `on_detached`.

### Step 4 — Drop the crutches

- Delete `ui_host::with_scope(F&&)`.
- Demote `push_scope()` to test-only (or delete entirely — check
  caller count).
- Remove `with_scope` wrapper in `widgets_demo::build_widgets_demo`.
- Trim `ui_context_fixture` if no remaining tests depend on its
  `scope_token` member.

### Step 5 — Docs + announcement

- Delete this doc's "proposal" banner (flip to "adopted").
- Update `ONYXUI_UI_HOST_DESIGN.md` to cross-reference the
  lifecycle contract.
- Update any widget authoring docs (if any exist) to mention
  `on_attached` as the preferred home for ambient-service setup.

---

## 10. Non-goals

- **Not adding a full React-style lifecycle.** No
  `componentWillMount` / `componentDidUpdate` / `shouldUpdate`
  equivalents. Just "attached/detached" hooks.
- **Not changing the layout pipeline.** `measure()` / `arrange()`
  are already fine as-is.
- **Not restructuring the renderer.** Render-time service lookups
  stay.
- **Not touching `event_target::handle_event`.** Event routing is
  orthogonal.
- **Not supporting reparenting.** If a subtree moves from one
  parent to another inside the same host, it does NOT fire
  detached + attached. (Add later if a consumer needs it; currently
  no widget reparents.)

---

## 11. Risks & rollback

| Risk | Mitigation |
|------|------------|
| `on_attached` ordering bugs (parent vs child) | Pre-order, explicit tree walk in `dispatch_attached`. Unit-test ordering in step 1. |
| Menu bar's hotkeys fail to register because `on_attached` runs before the bar has been inserted into the menu bar vbox | Attach fires when the **widget** enters the tree, not the widget's parent — menu_bar is the container, so `initialize_hotkeys` sees its own children already populated. Verified by the existing menu_bar ctor, which runs after its children are built. |
| `text_view` padding diverges visually after the port | Add a canvas-comparison test for a stock text_view before/after the port; fall back to render-time resolve if they differ. |
| Remove `with_scope` and discover a consumer we missed | Leave `push_scope` in place as the escape hatch until we've verified no caller remains in warlords. One CI cycle with `[[deprecated]]` annotations before deletion. |
| Tests that rely on the fixture's scope being active for non-widget reasons break | Keep `ui_context_fixture::m_scope` alive until the last such test is ported. Grep for direct `ui_services<B>::…` calls in test bodies. |

**Rollback:** each step is independently revertible. If step 3
(text_view) regresses, revert just that commit; steps 1 and 2
stand on their own. If the whole design turns out wrong, steps 1's
`on_attached` hook is a strict superset of the status quo (the
default is a no-op, nothing that previously worked breaks).

---

## 12. Acceptance criteria

- [ ] No widget constructor under `include/onyxui/widgets/` calls
      `ui_services<Backend>::…`.
- [ ] `ui_element` exposes `on_attached(ui_host&)` and
      `on_detached(ui_host&)` hooks with no-op defaults.
- [ ] `ui_host::mount()` / `unmount()` fire the hooks in pre-order /
      reverse pre-order respectively, inside a pushed scope.
- [ ] `ui_host::with_scope(F&&)` deleted. `push_scope()` either
      deleted or marked tests-only with a comment explaining why.
- [ ] `widgets_demo::build_widgets_demo` is a one-liner:
      `return std::make_unique<widgets_demo>(parent);`
- [ ] `ui_unittest` green modulo the pre-existing title-bar icon
      failures.
- [ ] warlords `bane` still builds and runs.
- [ ] Visual-equivalence check for `menu_bar` and `text_view` under
      default theme — before-and-after render diff clean.
