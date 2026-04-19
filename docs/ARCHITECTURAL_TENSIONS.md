# Overlay & Window Lifecycle Tensions in OnyxUI

> Status: living observation document
> Context: written after several rounds of bug-fixing in `onyx_ui`
> driven by regressions surfacing in the `warlords/bane` consumer.
> Updated after a second review sharpened the scope.

This document describes a cluster of design tensions concentrated in
**one seam of OnyxUI: overlay / window presentation, the layer
manager, and how consumers present dialogs**. Consumer integration
work exposed them through a recognizable "fix exposes next bug"
cadence, which is the signal that the problems are structural rather
than additive.

This document is deliberately scoped. It is **not** a critique of
OnyxUI as a whole.

---

## 1. What isn't broken

The retained widget-tree side of OnyxUI is in good shape for
consumers. Normal scene construction — build containers, add
children, keep widget handles, wire signals — reads as a standard
retained UI toolkit. The exemplar is
`warlords/src/bane/scenes/strategic_scene.cc`: a ~100-line scene with
menu bar, map view, sidebar, status bar assembled through
`add_child`/`emplace_child`, with no protective infrastructure, no
manual lifecycle choreography, no ambient-state gymnastics. That path
works as advertised.

The problems described below **do not appear** in code that stays
inside the tree: layout, widgets, themes, signals, input events,
containers. A consumer that never touches dialogs or overlays will
not hit them.

---

## 2. What is broken, and where

The fragile seam is:

- `window<Backend>` as a type,
- `layer_manager<Backend>` and its integration with `window`,
- the ambient `ui_services::layers()` lookup used by pre-existing
  presentation APIs,
- the ownership relationship between the scene, the dialog, the
  layer, and the ui_context that the layer lives in.

Of the four tensions detailed in §3, the two that drive the vast
majority of consumer-visible pain are **(D) presentation has no
owner** and **(A) `window` is both a tree widget and an overlay
widget**. Everything in §2 below — the teardown rituals, the boolean
flags, the window registries, the split ambient/explicit examples —
is protective machinery consumers reach for because the library
doesn't hand back an object that IS the presentation, and because
the single `window` type carries two separate hosting contracts at
once.

Evidence that consumers end up writing *protective infrastructure*
around exactly this seam:

1. **`scene_with_ui`** in bane exists largely to manage the
   `ctx_`/`ui_handle` teardown order so dialog windows don't outlive
   the context they reference. The header documents the member-order
   contract explicitly because nothing in OnyxUI enforces it
   (`warlords/src/bane/kernel/scene_with_ui.hh`).

2. **`init_scene`** manually tracks dialog presence with four
   `bool m_*_shown` flags and pairs every `show(layers())` with a
   matching `hide(layers())` in its transitions
   (`warlords/src/bane/scenes/init_scene.cc`). Workable, but
   boilerplate a consumer shouldn't need.

3. **The OnyxUI demo** maintains a global
   `std::vector<std::shared_ptr<window>>` window registry, with
   closed-signal cleanup, purely to keep spawned windows alive
   (`examples/widgets_demo/windows/window_registry.hh`). A library
   whose consumers have to bolt on their own shared_ptr registry is
   telling on itself.

4. **Split-API usage across examples and consumers.** The demo still
   uses the ambient `win->show()` pattern
   (`examples/widgets_demo/widgets_demo.hh`) while bane uses the
   newer `win->show(*layers)` pattern. Two styles side-by-side in the
   official examples means "blessed usage" is unclear.

5. **`main_window::create_window()` exists but nobody uses it.**
   Outside its own tests, examples construct windows manually. A
   helper with no consumers is a sign the helper doesn't match real
   usage.

6. **Tests knowingly embed `window` in a widget tree** even though
   the type is overlay-oriented
   (`unittest/widgets/test_window_title_bar_icons.cc`). When the
   unsupported path is the convenient one in tests, the API boundary
   isn't really enforced.

## 3. The four tensions at the seam

### (A) `window<Backend>` was two types, two coordinate systems, and two lifetime models pretending to be one

**Status as of WAR-47 (2026-04-19): sealed.**
`window<Backend>` still derives from `widget_container<Backend>`, but
the base class is now `protected`, not `public`. External tree code
cannot upcast a `window*` to a `ui_element*` — `parent->add_child(std::make_unique<window<B>>(...))`
fails to compile. The tree-hostable mode has been closed at the type
level; only the overlay mode remains reachable through the public
API. The re-exported widget interface (`using base::bounds;`
`using base::is_visible;` etc.) is the subset of the widget surface
window consumers legitimately call; the rest (`add_child`, direct
`parent()` manipulation, tree insertion) is gone.

Retained historical context, for posterity:

|              | Tree-hosted (was)             | Overlay-hosted (is)           |
|--------------|-------------------------------|-------------------------------|
| Position     | Parent layout authority       | Absolute viewport coords      |
| Clipping     | Parent content-area clip      | No parent clip                |
| Dragging     | Clipped at parent bounds      | Free within viewport          |
| Events       | Tree hit-test                 | Layer routing + modal block   |
| Coord space  | Parent-relative               | Absolute viewport             |
| Lifetime     | `unique_ptr` chain            | `weak_ptr` + owner elsewhere  |

With the tree-hosted column walled off, the silent-axis-crossing
bugs that produced §4 rows 1–3, 5–7 can no longer recur through this
path. `window::bounds()` is now unambiguously absolute-viewport; the
cleanup path is always the layer_manager path; the coordinate system
is fixed at the type level.

### (B) Ambient lookups vs explicit parameters coexist without a rule

`ui_services<Backend>::layers() / themes() / input()` are thread-local
scope-stack singletons. Some APIs take dependencies explicitly
(`layer_manager::render_all_layers(renderer, viewport, theme, metrics)`);
others rely on ambient lookup (old `window::show()`, maximize
viewport fallback, system-menu popup). The mix surfaces in two
places:

- Test fixtures that isolate via `scoped_ui_context` (the visual
  harness) silently break ambient theme lookup.
- New consumers can't tell which entry points are ambient and which
  are explicit — both compile, both look the same.

### (C) Cross-entity lifetime is hand-rolled per relationship

OnyxUI has two distinct mechanisms for cross-object references, and
uses them inconsistently:

| Relationship                     | Today's mechanism                                  |
|----------------------------------|----------------------------------------------------|
| layer_manager → element          | weak_ptr + scoped_connection on element's `destroying` |
| element → parent                 | Raw pointer, no observer                           |
| window → owning layer_manager    | Raw pointer + `destroying` signal (added 2026-04)  |
| window → workspace               | Raw pointer, no observer                           |
| scene → ui_context               | `unique_ptr` + explicit on_exit teardown           |

Consumers inherit the weakest link. The `scene_with_ui` member-order
ritual exists because the window↔context relationship has no explicit
lifetime protocol; the workspace reference still doesn't have one
today.

### (D) Presentation has no owner

`window::show(layers)` adds a layer. The consumer is then responsible
for calling `hide(layers)` before either side goes away. This is why
`init_scene` ends up with `m_*_shown` booleans, why the demo needs a
registry, and why `scene_with_ui` needs its careful teardown. The
library doesn't hand back an object that *is* the presentation, so
consumers have to simulate one with flags.

---

## 4. The "fix N exposes N+1" pattern

A chronological list of overlay/lifecycle bugs found during the bane
integration:

| #  | Symptom                                                            | Root (from §3) |
|----|--------------------------------------------------------------------|----------------|
| 1  | `window::show()` silently produced 0×0 windows                      | B              |
| 2  | Dialog drag clipped by parent content area                          | A              |
| 3  | `main_window::create_window` passed central widget as `parent`      | A              |
| 4  | `scene_with_ui::on_exit` destroyed ctx before derived dialogs       | C              |
| 5  | `show()` on manager X then Y orphaned layer in X                    | C              |
| 6  | Manager destroyed before window → UAF on `m_layer_owner`            | C              |
| 7  | `maximize()` used parent-relative workspace as absolute             | A + C          |
| 8  | Visual test fixture nests a context and loses theme                 | B              |

Each was fixed correctly in isolation. But the cadence — find bug,
fix it, next one surfaces a few days later — is the diagnostic for
structural tension, not unrelated defects. Independent bugs don't
chain like that.

---

## 5. Proposed remediation, in priority order

These four moves close the specific tensions at the seam. They
compose: doing them in order means each earlier fix shrinks the scope
of the later ones.

### Priority 1 — Make overlay presentation RAII-owned

**Tension addressed:** D, collateral win for C.

Give consumers a type that IS the presentation. Owning the presenter
owns the layer registration AND (optionally) the window itself. No
more manual show/hide/boolean pairs; no more lifetime-gymnastics in
the consuming scene; no more registry in the demo.

Sketch:

```cpp
template<UIBackend B>
class presented_window {
public:
    presented_window(layer_manager<B>&, std::unique_ptr<window<B>>,
                     presentation_kind = modal);
    ~presented_window();                       // removes layer, drops window

    window<B>* get() noexcept;
    [[nodiscard]] bool is_visible() const noexcept;

    presented_window(const presented_window&) = delete;
    presented_window(presented_window&&) noexcept;
};
```

Consumer usage becomes:

```cpp
// in scene state:
std::unique_ptr<presented_window<B>> m_selector;

// show:
m_selector = std::make_unique<presented_window<B>>(layers(), build_selector());

// hide:
m_selector.reset();

// scene torn down:
// m_selector drops automatically, layer removed, window destroyed — no ritual
```

This helper removes most of the consumer-side pressure: it eliminates
the init_scene boolean tracking and the demo's window registry
outright, and makes the `scene_with_ui` destruction-order comment
mostly redundant. "Mostly" because the comment is only fully
obsolete once:

- the presenter exclusively owns the overlay object (no raw
  cross-references back into scene/context state from dialog
  internals), and
- every long-lived dialog is held through a presenter rather than
  constructed and retained directly by the scene.

Get those right and the ordering contract stops existing as a
runtime concern. Miss them and you've moved the contract from
`scene_with_ui` into individual dialog classes, which is no better.

Estimated effort: small. `scoped_layer` already exists as the
non-owning version; this is mostly adding widget ownership and a
typed entry point, plus an audit pass for dangling scene↔dialog
back-references.

### Priority 2 — Finish the ambient-to-explicit migration

**Tension addressed:** B.

Concrete steps:

- Delete the deprecated parameterless `window::show()` and
  `window::show_modal()` — not mark, delete. The wrappers have been
  deprecated long enough and they hide which of two patterns a call
  site uses.
- Port every example (`examples/widgets_demo/**`) to the explicit
  form.
- Port the remaining tests that use the ambient pattern. `test_window_show.cc`
  and `test_window_modal.cc` are the obvious ones.
- Similarly audit the window internals that still do
  `ui_services<Backend>::layers()` for anything that can take the
  dependency explicitly (the maximize viewport fallback; the
  title-bar system-menu popup).

After this, the rule is simple and consistent: public presentation
APIs take their dependencies explicitly; ambient services exist
only for bookkeeping that genuinely can't plumb a reference (render
pipeline internals, destructor cleanup).

### Priority 3 — Make `window<B>` overlay-only at the type level

**Tension addressed:** A, collateral win for parts of D.

The move:

- Keep the name `window<B>`.
- Change its base class: `window<B>` no longer derives from
  `widget_container<B>` (and transitively, no longer from
  `ui_element<B>`). It becomes a value-like host object that owns
  its internal widget tree as a composed member.
- `layer_manager` is updated to host the new overlay type — likely
  via a small `get_root()` accessor the manager calls for rendering,
  rather than `add_layer` taking `ui_element*` directly.
- The test sites in `test_window_title_bar_icons.cc` that today
  `emplace_child<window>` into a `panel` are migrated to the
  `set_workspace(panel.get())` pattern `test_window.cc` already
  demonstrates. That's what those tests were really asking for —
  they embedded in a tree only because the legacy maximize path
  consulted `parent()`, a path we already removed.

What we do NOT need, which an earlier revision of this document
suggested:

- **A new tree-hosted framed widget** (`pane`, `inline_window`,
  `framed_container`). Investigation of actual usage showed zero
  consumer demand for it: bane has no in-tree window; every
  onyx_ui example uses overlay mode; and the three in-tree uses in
  tests are all testing `maximize()` behavior through a scaffold
  parent, not asking for a tree-hosted window-chrome widget per se.
  If a consumer ever does want a framed container in a layout tree,
  `group_box<B>` already exists and already does that — no new type
  is needed.

After the move, the rule is compile-time: anyone who writes
`parent->add_child(std::make_unique<window<B>>(...))` gets a type
error. The ambiguity the type carried for months becomes impossible
to express.

Estimated effort: ~1 day focused onyx_ui work. The base-class change
is localized, the `layer_manager` integration is small, and there
are only a handful of test sites to migrate. Consumer-side impact is
minimal because every real consumer is already overlay-only.

### Priority 4 — Decide `main_window::create_window` fate

**Status as of WAR-48 (2026-04-19): deleted.**

`main_window::create_window` was used only by its own tests; every
other consumer built windows directly with `std::make_unique` and
called `set_workspace(central_widget())`. A convenience helper that
nobody reaches for is negative value — it appears in docs, creates
discovery friction, and its existence implies it's the right path
when in practice consumers skip it.

Post-WAR-47 the helper was a two-line wrapper
(`make_shared<window>(args...); set_workspace(central)`). Deleting
it is the honest move: `main_window` stays purely a chrome
container, and the `set_workspace(central_widget())` idiom is
explicit at every call site. The idiom is short enough that it
doesn't warrant abstraction, and with `window<B>` now overlay-only
(WAR-47), pairing a window to its workspace is a one-time decision
the consumer makes anyway.

The `presented_window<B>` / `show(lm)` APIs handle the presentation
lifetime; `main_window` just provides the chrome and the workspace
reference via `central_widget()`.

### Pragmatic vs. architectural ordering

The numbering above is the order I'd execute the changes in.
Separately:

- **Lowest-risk progress** is P1 → P2. Both are additive and
  mechanical, both remove concrete pain points consumers feel
  today, neither requires breaking-change budget.
- **The architecturally correct cure** is P3. P1 + P2 remove the
  *immediate* bugs, but unless the type split also lands, OnyxUI
  keeps carrying the conceptual ambiguity — `window` is still two
  things and the next new feature at the seam will re-open the
  pattern. If there's capacity for one breaking change, this is
  where to spend it, and it should come soon after P1, not in some
  indefinite future.

---

## 6. What this is NOT

- **Not a claim that OnyxUI is broadly bad.** The retained widget
  tree works well; this document is specifically about the
  overlay/presentation seam.
- **Not a bug tracker.** The eight symptoms in §4 have been fixed.
  This document is about why they clustered.
- **Not a commitment.** The priorities above are informed
  recommendations. Each has a cost that has to be weighed against
  current OnyxUI development bandwidth.

---

## 7. Things to avoid

### Don't add new raw cross-references without a lifetime protocol

If a new type needs to reference another across ownership, pick the
pattern: weak_ptr for shared ownership, or raw-pointer +
`scoped_connection` on a `destroying` signal for non-shared. Don't
invent a third variant; don't rely on "it usually works because the
destructor order is X".

### Don't use deprecation markers as permanent documentation

`[[deprecated("use X")]]` is fine as short-term migration aid.
Without a removal target, it becomes permanent noise — readers learn
to scroll past warnings and the migration intent is lost. If a
deprecated API is kept longer than two release cycles, either delete
it or un-deprecate it. Priority 2 forces this for show/show_modal.

### Don't rely on implicit lifetime ordering

The `scene_with_ui::on_exit` bug was silent-works-most-of-the-time
for months, because C++'s member-reverse-declaration-order rule
happened to save it in practice. When a destructor needs another
object alive, make the dependency explicit (signal subscription,
owning relationship) rather than betting on member order.

### Don't fix coordinate-space bugs by sprinkling
`get_absolute_logical_bounds()` at call sites

Works, but fragile — every new API has to remember the convention.
Priority 3 mostly dissolves this by separating the types that cross
the boundary; any remaining work is small.

### Don't expand the surface area of the overlay system

Until priorities 1 and 3 land, every new overlay-related feature
inherits today's tensions and will show up as another item on the
"fix N exposes N+1" list. Better to land the structural fix first.

---

## 8. References

Key files discussed:

- `include/onyxui/widgets/window/window.hh` + `.inl` — the `window`
  class.
- `include/onyxui/services/layer_manager.hh` — the layer manager.
- `include/onyxui/widgets/main_window.hh` + `.inl` — `main_window`
  and `create_window`.
- `include/onyxui/core/raii/scoped_layer.hh` — non-owning RAII layer
  handle; starting point for Priority 1.
- `include/onyxui/services/ui_context.hh` — scoped context stack.
- `include/onyxui/ui_handle.hh::display_impl` — root render + layer
  render composition.
- `unittest/utils/visual_test_helpers.hh::render` — harness that
  collides with ambient-lookup convention.

Consumer-side evidence cited in §2:

- `warlords/src/bane/scenes/strategic_scene.cc` — clean retained-UI
  scene construction (the happy path).
- `warlords/src/bane/kernel/scene_with_ui.hh` — member-order
  destruction ritual.
- `warlords/src/bane/scenes/init_scene.cc` — overlay-lifecycle
  boilerplate (shown-flags, paired show/hide).
- `examples/widgets_demo/windows/window_registry.hh` — demo
  window-lifetime registry.
- `examples/widgets_demo/widgets_demo.hh` — still using the ambient
  `win->show()` style.
- `unittest/widgets/test_window_title_bar_icons.cc` — test that
  embeds a window in the widget tree.

Related existing design docs in `docs/`:

- `LOGICAL_UNITS_DESIGN.md`
- `EVENT_ROUTING_IMPLEMENTATION_PLAN.md`
- `MAIN_WINDOW_IMPLEMENTATION.md`
