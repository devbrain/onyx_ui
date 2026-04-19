# OnyxUI Runtime Direction

> **Status: adopted (2026-04-19).** This is OnyxUI's canonical product
> direction. Implementation work and future API proposals should line
> up with the thesis in sections 3 and 5.
>
> The companion docs `ONYXUI_API_SKETCH_AND_MIGRATION.md` and
> `ONYXUI_IMPLEMENTATION_PLAN.md` are inputs to this direction, not
> decisions. Specifically, their proposals for a `pane<B>` type, the
> `ui_runtime` / `ui_surface` / `overlay_window` / `presented_overlay` /
> `presented_dialog` renames, and the standalone app-shell layer
> (`app<B>` / `app_window<B>` / `dialog<B>`) are **not adopted** — none
> of them has a concrete consumer need today. They may be revisited
> if and when a real consumer asks.
>
> Audience: OnyxUI maintainers and consumers embedding OnyxUI into
> custom hosts.
>
> Motivation: define what OnyxUI is trying to be, so API work
> converges toward a coherent product rather than a sequence of local
> fixes.

---

## 0. Current state (2026-04-19)

Most of §8's architectural changes were delivered in the WAR-44 epic.
What shipped:

| Proposal (§8)                                   | Status                    | Where                |
|-------------------------------------------------|---------------------------|----------------------|
| 8.1 RAII-owned presentation                     | ✅ shipped                | WAR-45 — `presented_window<B>` |
| 8.2 Split overlay windows from tree panes       | 🟡 partial                | WAR-47 — `window<B>` sealed via protected inheritance; no `pane<B>` added (no consumer asked) |
| 8.3 Finish ambient-to-explicit separation       | ✅ shipped                | WAR-46 — deprecated forms deleted; `PRESENTATION_CONVENTIONS.md` published |
| 8.4 Reduce raw cross-references                 | ✅ shipped                | `layer_manager::destroying` + `window::m_layer_owner_conn` |
| 8.5 Collapse startup into one obvious path      | ⬜ not started             | Waiting on a consumer need — warlords is satisfied |

The `main_window::create_window` helper is gone (WAR-48). The signal
primitive is now safe against self-destruction during emit (fix landed
post-WAR-48).

Not adopted from the companion docs:

- **`pane<B>` type split** — the WAR-47 investigation found zero
  consumer need; `group_box<B>` already covers the tree-hosted framed
  container role. Deferred until a real consumer asks.
- **Renames** (`ui_runtime`, `ui_surface`, `overlay_window`,
  `presented_overlay`/`presented_dialog`) — pure churn with no
  semantic gain over the current names.
- **App-shell layer** (`app<B>` / `app_window<B>` / `dialog<B>`) —
  speculative; no consumer needs it. warlords explicitly doesn't want
  it. widgets_demo is a demo, not a product.

Remaining work broadly consistent with this direction:

- `scene_with_ui` teardown-comment cleanup (mentioned in §12 Phase 1
  success criteria but never explicitly addressed as a task)
- The 5 pre-existing title-bar visual-render test failures
- Compositional split of `window<B>` away from `widget_container<B>`,
  if someone wants to turn the protected-inheritance seal into a
  true non-element type

---

## 1. Executive summary

OnyxUI should not aim to become "another widget toolkit".

It should aim to become a **retained UI runtime for custom hosts**, with:

- a **core runtime** that is well-suited to game engines, simulations, tools, and custom render loops,
- an **embedding API** for hosts such as `warlords/bane`,
- a **high-level app shell** for simpler standalone desktop-style tools.

This direction fits the strongest parts of the current codebase:

- retained widget tree,
- layout,
- rendering abstraction,
- event routing and focus,
- theming,
- popups / overlays / dialogs.

It also matches where the codebase is weakest today: not in ordinary retained-tree UI, but at the seam where overlay presentation, window lifecycle, layer management, and consumer ownership meet.

The central design move is:

> Treat overlay presentation as a first-class runtime subsystem, not as an incidental coupling between `window`, `layer_manager`, and ambient services.

---

## 2. Why this document exists

The current architecture note in [ARCHITECTURAL_TENSIONS.md](./ARCHITECTURAL_TENSIONS.md)
describes the overlay/window lifecycle pain honestly and correctly. This document goes one level up:

- what OnyxUI should be as a product,
- what public API tiers it should expose,
- how that settles with game-engine consumers like `warlords`,
- what concrete subsystem changes are needed to get there.

This is intentionally broader than a bug fix plan, but still constrained by the current code and current consumers.

---

## 3. Product position

### 3.1 What OnyxUI should be

OnyxUI should be positioned as:

> A retained UI runtime for custom hosts, with a simple app shell on top.

That means the primary value is not "we have buttons and labels". The primary value is:

- retained widget tree,
- deterministic layout,
- backend-agnostic logical coordinates,
- explicit rendering integration,
- event routing and focus management,
- theming and styling,
- overlay / popup / dialog presentation,
- host embedding into nonstandard application loops.

This is a better fit for:

- game engines,
- simulation frontends,
- editors and internal tools,
- custom rendering backends,
- desktop applications when needed.

### 3.2 What OnyxUI should not be

OnyxUI should not optimize itself around becoming:

- "Qt-lite",
- "FLTK clone",
- a generic desktop-first toolkit whose design assumes the toolkit owns the whole application.

That would push the public API toward desktop assumptions that are wrong for real engine embeddings.

---

## 4. Evidence from current consumers

### 4.1 What works well today

When OnyxUI is used as a retained widget tree, the code reads well.

`warlords/src/bane/scenes/strategic_scene.cc` is the reference example:

- build containers,
- add children,
- keep a few widget handles,
- connect signals,
- render through the host.

That path looks normal and productive.

### 4.2 Where consumers are paying the cost

The pain is concentrated in the overlay/presentation seam:

- `warlords/src/bane/kernel/scene_with_ui.hh`
  has to document destruction order so windows do not outlive the UI context they implicitly depend on.
- `warlords/src/bane/scenes/init_scene.cc`
  manually tracks overlay visibility with `m_*_shown` booleans and explicit `show(layers())` / `hide(layers())` pairs.
- `examples/widgets_demo/windows/window_registry.hh`
  introduces a global `shared_ptr` registry purely to keep spawned windows alive.
- `examples/widgets_demo/widgets_demo.hh`
  still uses ambient `win->show()` while `warlords` uses explicit `show(layers())`, so the preferred public pattern is unclear.
- Tests such as `unittest/widgets/test_window_title_bar_icons.cc`
  still embed `window` inside the tree because that is convenient, even though the type is increasingly overlay-oriented.

This is the signal that OnyxUI is not suffering from "some bugs in windows". It is suffering from a design boundary that is not explicit enough.

---

## 5. Core design thesis

OnyxUI needs **two public faces built on one core**.

### 5.1 Face A: embedding-first runtime API

This is the API for:

- game engines,
- custom scene systems,
- realtime editors,
- any host that already owns the renderer, window, and main loop.

Properties:

- explicit dependencies,
- deterministic host integration,
- no assumption that OnyxUI owns the app loop,
- compatible with custom teardown rules and scene stacks.

### 5.2 Face B: simple standalone app shell

This is the API for:

- demos,
- internal tools,
- quick utilities,
- desktop-style apps that do not need custom host integration.

Properties:

- opinionated defaults,
- one obvious startup path,
- one obvious top-level window path,
- minimal explicit plumbing.

### 5.3 Shared core

Both faces should share one runtime:

- widget tree,
- layout,
- theming,
- input routing,
- overlay runtime,
- backend abstractions.

The high-level app shell is a convenience layer over the runtime, not a different product.

---

## 6. Target architecture

The architecture should be split conceptually into three layers.

## 6.1 `onyxui-core`

This is the actual runtime engine.

Responsibilities:

- `ui_element` and the widget tree,
- layout strategies and measurement/arrangement,
- rendering context abstractions,
- focus and input routing,
- hotkeys and actions,
- theming and style resolution,
- overlay and popup runtime internals.

This layer should remain general and reusable.

## 6.2 `onyxui-embed`

This is the host integration layer.

Responsibilities:

- `ui_context`,
- `ui_handle` or successor surface abstraction,
- backend metrics,
- explicit render entry points,
- explicit event ingestion,
- overlay presentation within a host-owned render loop.

This is the layer `warlords` should consume.

## 6.3 `onyxui-app`

This is the convenience layer.

Responsibilities:

- app bootstrap,
- default event loop,
- top-level app window,
- modal dialogs,
- desktop-style helper APIs.

This layer should hide the embedding internals unless the user opts into advanced usage.

---

## 7. The main architectural problem to solve first

The most important subsystem to redesign is:

> overlay presentation and window lifecycle

Why this comes first:

- it is where consumers are writing compensating code,
- it is where type ambiguity currently lives,
- it is where lifetime and coordinate-space bugs keep clustering,
- it blocks a clean public API for both standalone apps and embedded hosts.

Until that seam is fixed, any higher-level consumer API will either:

- expose the same fragility in a friendlier wrapper, or
- hide it temporarily while the complexity remains in the core.

---

## 8. Required architectural changes

## 8.1 Make presentation RAII-owned

### Problem

Today, `window::show(...)` creates a presentation side effect, but the presentation itself has no owning object.

Consequences:

- consumers invent `m_*_shown` booleans,
- consumers must remember to call `hide(...)`,
- teardown ordering becomes delicate,
- demo code needs an out-of-band registry to keep windows alive.

### Requirement

Introduce an owning presentation type, for example:

```cpp
template<UIBackend B>
class presented_overlay;

template<UIBackend B>
class presented_dialog;
```

Properties:

- owns the layer registration,
- may optionally own the overlay content object itself,
- removes the layer in its destructor,
- can expose `get()` for widget access,
- can answer `is_visible()`,
- move-only.

### Why this matters

This is the single highest-leverage simplification for consumers.

It makes the consumer lifecycle obvious:

```cpp
std::unique_ptr<presented_dialog<B>> m_connect;

// show
m_connect = ui().present_modal(build_connect_dialog());

// hide
m_connect.reset();
```

Instead of:

- separate window ownership,
- separate layer registration,
- separate booleans,
- separate hide calls.

## 8.2 Split overlay windows from tree panes

### Problem

`window<Backend>` is currently trying to act as both:

- tree-hosted titled container,
- overlay-hosted draggable/modal/top-level presentation object.

Those are not the same concept.

### Requirement

Split them into two public types.

Suggested shape:

- `overlay_window<B>`
  - top-level overlay semantics
  - drag/maximize/modal/presentation
  - no tree hosting by parent containers
- `pane<B>`
  - title/border/content container
  - tree widget
  - no modal semantics
  - no top-level presentation
  - no drag outside tree rules

### Why this matters

This makes the boundary explicit in the type system instead of leaving it as a documentation convention.

It also reduces coordinate-space and clipping bugs because the two modes no longer share one implementation surface.

## 8.3 Finish ambient-to-explicit separation

### Problem

OnyxUI currently exposes a mix of:

- explicit APIs,
- ambient service lookups,
- deprecated wrappers that still appear in examples and tests.

That gives consumers no stable mental rule.

### Requirement

Define one rule:

- embedding-facing APIs are explicit,
- app-shell-facing APIs may be implicit/opinionated,
- the same concept should not be public in both forms at once.

Concretely:

- embedding APIs should take dependencies explicitly,
- app-shell APIs should wrap that internally and present a simple surface,
- deprecated parameterless presentation APIs should eventually be removed.

### Why this matters

This removes ambiguity and makes examples teach one coherent style per API tier.

## 8.4 Reduce raw cross-references

### Problem

Cross-object lifetime currently uses multiple patterns:

- weak_ptr in some places,
- raw pointer + destroying signal in others,
- implicit destruction order in others.

### Requirement

Adopt a clear rule:

- shared ownership graphs use smart pointers or handles,
- non-owning cross-references require a destruction protocol,
- public consumer workflows should not rely on implicit member-order tricks.

### Why this matters

Consumers should not have to understand internal lifetime topology to use dialogs safely.

## 8.5 Collapse startup into one obvious path per API tier

### Problem

The low-level pieces (`scoped_ui_context`, `ui_handle`, service lookups, event loop plumbing) are too visible even in examples.

### Requirement

Provide:

- one obvious standalone path,
- one obvious embedding path.

Example shapes:

Standalone:

```cpp
onyxui::app app;
onyxui::app_window win("Title", 1024, 768);
win.set_content(build_ui());
win.show();
return app.run();
```

Embedded:

```cpp
onyxui::ui_runtime<B> ui(metrics);
onyxui::ui_surface<B> surface(ui, build_ui());
surface.render(renderer);
surface.handle_event(ev);
```

The exact names can differ; the important part is reducing visible plumbing.

---

## 9. Public API model

The API should be documented as two supported entry styles.

## 9.1 Simple app-shell style

For apps and tools:

- `app`
- `app_window`
- `dialog`
- `message_box`
- `confirm_dialog`
- standard widgets

Consumer should rarely touch:

- `layer_manager`,
- `ui_context`,
- `ui_services`,
- render internals.

## 9.2 Embedding style

For engines and host runtimes:

- `ui_runtime` / `ui_context`
- `ui_surface` / `ui_handle`
- explicit render/update/event ingestion
- `presented_overlay` / `presented_dialog`

Consumer should be able to:

- own the event loop,
- own the renderer,
- integrate with scene transitions,
- mount/unmount UI surfaces deterministically.

`warlords` should map to this tier.

---

## 10. How this should settle with `warlords`

`warlords` should not be forced into the high-level app-shell API.

It already has:

- its own scene stack,
- its own renderer,
- its own event loop,
- its own scene lifecycle.

That is normal for a game engine consumer.

### 10.1 What should stay

`scene_with_ui` or its successor should continue to exist as the embedding seam:

- create the runtime/context,
- host the root widget tree,
- render via the engine renderer,
- forward events from the engine.

This is the correct role of an embedding API.

### 10.2 What should change

The dialog/overlay boilerplate in `init_scene` should disappear into RAII presentation ownership.

Instead of:

- `m_*_shown` booleans,
- separate owned dialog objects,
- explicit `show(layers())`,
- explicit `hide(layers())`,

`warlords` should hold presentation objects directly:

```cpp
std::unique_ptr<presented_dialog<warlords_backend>> m_connect;
std::unique_ptr<presented_dialog<warlords_backend>> m_import;
```

That should:

- remove most of the current overlay state bookkeeping,
- reduce lifecycle fragility,
- make scene teardown straightforward.

### 10.3 Why `warlords` is the reference consumer

`warlords` is valuable because it is not a toy app:

- it exercises embedding, not just standalone app startup,
- it has real scene transitions,
- it uses overlays in realistic ways,
- it has already flushed out structural issues.

If OnyxUI works cleanly in `warlords`, the embedding API is probably good.

---

## 11. Why not just make it "dead simple like FLTK"

FLTK-like simplicity is attractive, but it comes from being much more opinionated:

- one obvious app model,
- one obvious event loop,
- one obvious top-level window model,
- fewer public composition seams.

OnyxUI should learn from that, but not copy it wholesale.

If OnyxUI optimizes exclusively for FLTK-like simplicity, it will degrade the engine-embedding story.

The correct compromise is:

- FLTK-like simplicity at the `onyxui-app` layer,
- explicit and host-friendly control at the `onyxui-embed` layer.

That gives both classes of consumers what they actually need.

---

## 12. Concrete roadmap

The roadmap was staged so each step improves consumer experience
without requiring a big-bang rewrite. Phases 1–2 are shipped; phase 3
landed in a different form (seal via protected inheritance rather than
a new `overlay_window` / `pane` type split); phases 4–5 are not
adopted in their proposed form (see §0).

## Phase 1: stabilize overlay ownership — ✅ shipped (WAR-45)

Deliverables:

- add `presented_overlay` / `presented_dialog`,
- make layer registration RAII-owned,
- port `warlords` overlay-heavy scenes to it,
- port demos away from global window registries where possible.

Success criteria:

- no `m_*_shown` flags in `warlords` overlay consumers,
- no demo-side registry whose purpose is "keep windows alive until close",
- fewer explicit show/hide pairings in consumers.

## Phase 2: clean up public presentation API — ✅ shipped (WAR-46)

Deliverables:

- remove or fully isolate deprecated ambient presentation entry points,
- convert examples to one consistent style,
- update tests to match the chosen API tier intentionally.

Success criteria:

- examples no longer teach conflicting patterns,
- a new consumer can tell what the intended presentation API is without reading implementation details.

## Phase 3: split the types — 🟡 delivered in reduced form (WAR-47)

Delivered:

- `window<B>` sealed via `protected` inheritance of
  `widget_container<B>`; external code can no longer upcast to
  `ui_element*`, so `parent->add_child(std::make_unique<window<B>>(...))`
  is a compile error.
- `layer_manager<B>` friended so the one legitimate upcast path
  (registering a window as an overlay layer) still works.
- Three tree-hosted test sites migrated to `set_workspace(panel.get())`.

Not delivered (intentional):

- No `overlay_window<B>` public type — the existing `window<B>` name
  was kept; semantics are now exclusively overlay, and the guardrail
  is at the type system, not at a name.
- No `pane<B>` — the WAR-47 investigation found zero consumer need;
  `group_box<B>` already covers the tree-hosted framed container role.

Success criteria met:

- impossible to accidentally `add_child` an overlay window,
- coordinate-space and clipping ambiguities resolved at the axes that
  consumers actually hit,
- tests that relied on tree-hosting `window` ported to the workspace
  pattern.

## Phase 4: define the app shell — ⬜ not adopted

The app-shell proposal (`app<B>` / `app_window<B>` / `dialog<B>`) is
not adopted in its current form. Rationale:

- warlords — the reference embedding consumer — explicitly doesn't
  want app-shell semantics,
- widgets_demo is a demo harness, not a product; it doesn't create
  consumer demand on its own,
- there is no third consumer asking for this layer.

Revisit when a concrete standalone-tool consumer emerges. Until then,
§6.3 `onyxui-app` remains a direction, not a scheduled workstream.

## Phase 5: standard dialogs and utility surfaces — ⬜ not adopted

Same rationale as Phase 4 — no consumer is asking for prepackaged
message / confirm / input dialogs today. Revisit when one does. The
existing `dialog<B>` + `window_presets.hh` helpers are adequate for
the current consumer base.

---

## 13. Documentation implications

The documentation set should eventually be reorganized by audience:

- **Core/runtime design**
- **Embedding into custom hosts**
- **Standalone app shell**
- **Widget authoring**
- **Theming/styling**

In particular, examples should be split explicitly:

- "standalone app examples"
- "engine embedding examples"

Right now the docs and demos mix those concerns too often.

---

## 14. Non-goals

This direction does **not** imply:

- replacing the retained widget-tree architecture,
- abandoning desktop apps,
- requiring a giant rewrite before any consumer benefit appears,
- removing low-level APIs that advanced consumers genuinely need.

It **does** imply:

- narrowing the default public story,
- making ownership and presentation explicit,
- preferring strong API boundaries over flexible-but-ambiguous ones.

---

## 15. Decision summary

If OnyxUI wants to be suitable for both game engines and desktop apps, the right shape is:

- **core retained runtime**
- **embedding-first host API**
- **simple app shell on top**

The first subsystem to redesign is overlay presentation.

The first product-level principle is:

> The normal consumer should not manage layer registration, visibility bookkeeping, and lifetime choreography by hand.

If that principle becomes true, OnyxUI gets materially easier to use in both:

- engine consumers like `warlords`,
- simpler desktop-style apps and demos.

That is the shortest path from the current codebase to a coherent runtime product.

