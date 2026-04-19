# Presentation API conventions

> **Tracking:** WAR-46 closed this. Companion to `ARCHITECTURAL_TENSIONS.md`
> § 5 P2.

## The rule

**Public presentation APIs take their dependencies explicitly.**
Specifically: `window::show`, `window::show_modal`, `window::hide(lm)`,
`dialog::show_modal`, and the `window_presets.hh` helpers
(`show_info`, `show_confirm`, `show_warning`, `show_error`) all require
a `layer_manager<Backend>&` argument. The parameterless overloads are
gone — not deprecated, deleted.

Consumers route through `presented_window<Backend>` when they want
RAII ownership (the common case), or call the `(layer_manager&, ...)`
entry points directly when they need a bare lifetime.

## Where ambient lookups still live

`ui_services<Backend>::layers() / themes() / input() / metrics()`
remain, but they are *internal* services consumed by the render /
event / layout pipeline, not part of the presentation API surface.

Two explicit exceptions inside `window<Backend>` still consult
`ui_services<Backend>::layers()`:

1. **Maximize viewport fallback** (`window::maximize`) — when a
   floating window has no workspace set, maximizing fills the viewport.
   The viewport is a shared frame-level resource with no per-window
   alternative: there is no `m_layer_owner->get_viewport()` that makes
   sense for a window that isn't currently shown.

2. **Drag-clamp viewport fallback** (`window::clamp_drag_bounds`) —
   same rationale as above, for the no-workspace case.

Both are commented at the call site and route through the ambient
services *only* when a workspace alternative isn't available.

### Intentionally NOT ambient

`window::hide()` (parameterless) uses `m_layer_owner` — the manager
recorded at show time — not the ambient lookup. It's parameterless
because it's called from the destructor and from `close()`, neither of
which can plumb a `layer_manager&` through. If the owner has already
been torn down (observed via `layer_manager::destroying`), `hide()`
degrades to a visibility-flag update; no layer goes orphaned.

`window`'s system-menu popup routes through `m_layer_owner` so that
nested scope-stack contexts don't send the popup to the wrong manager.

## Why the distinction matters

The previous mix — some public APIs ambient, some explicit — produced
two failure modes:

* **Silent misrouting.** A consumer in a nested `scoped_ui_context`
  called `win.show()`; the window registered with the inner manager's
  layer stack, not the outer one it was supposed to belong to. The bug
  was invisible until the outer context was destroyed first.

* **Ownership ambiguity.** Readers couldn't tell which overload of
  `show` was "blessed". The presence of both forms meant every example
  and every test picked one semi-randomly. The examples and the
  documented-best-practice drifted.

Explicit parameters eliminate both. If a consumer has a
`layer_manager&`, they know *which* manager. If they don't have one,
the API won't let them call the function, and that's a useful
compile-time nudge toward `presented_window`.

## Migration note

Code written against the pre-WAR-46 API must:

* Replace `win.show()` with `win.show(layers)`.
* Replace `win.show_modal()` with `win.show_modal(layers)` (an
  optional `dialog_position` argument is still accepted after the
  manager).
* Replace `dlg.show_modal(cb)` with `dlg.show_modal(layers, cb)`.
* Replace `show_info(msg)` (and the other preset helpers) with
  `show_info(layers, msg)`.
* For RAII lifetime — the common case — prefer constructing a
  `presented_window<Backend>` instead of calling `show*` directly; see
  `widgets_demo/widgets_demo.hh` for the pattern.
