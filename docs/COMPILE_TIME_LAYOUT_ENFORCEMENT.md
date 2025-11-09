# Compile-Time Layout Strategy Enforcement

**Problem:** Containers without layout strategies measure to {0,0} and don't render, but this bug isn't caught until runtime (or visual tests).

**Goal:** Prevent "missing layout strategy" bugs at **compile time** instead of runtime.

---

## Option 1: Deleted Default Constructor (Strongest Compile-Time Guarantee)

### Approach

Make `widget_container` require layout strategy in constructor:

```cpp
template<UIBackend Backend>
class widget_container : public widget<Backend> {
protected:
    // FORCE layout strategy at construction
    explicit widget_container(
        std::unique_ptr<layout_strategy<Backend>> layout_strategy,
        ui_element<Backend>* parent = nullptr
    )
        : base(parent)
    {
        this->set_layout_strategy(std::move(layout_strategy));
    }

    // DELETE default constructor - compile error without layout!
    widget_container() = delete;
};
```

### How It Works

Derived classes MUST pass layout strategy to base:

```cpp
template<UIBackend Backend>
window<Backend>::window()
    : widget_container<Backend>(
        std::make_unique<linear_layout<Backend>>(direction::vertical, 0),
        nullptr  // parent
      )
{
    // Constructor body...
}
```

**Compiler error if you forget:**
```
error: use of deleted function 'widget_container<Backend>::widget_container()'
note: declared here: widget_container() = delete;
```

### Pros
✅ **100% compile-time enforcement** - impossible to create container without layout
✅ Clear compiler error message
✅ Self-documenting API (constructor signature shows requirement)
✅ No runtime overhead

### Cons
❌ **BREAKING CHANGE** - requires updating ALL existing containers:
  - window (✅ already fixed)
  - panel
  - vbox, hbox
  - grid
  - anchor_panel, absolute_panel
  - menu, menu_bar
  - All custom user containers

❌ Less flexible (can't defer layout strategy initialization)
❌ Makes initialization order more complex (must construct layout before base)

### Effort: HIGH (2-3 hours to update all containers)

---

## Option 2: Static Analyzer Rule (No Code Changes)

### Approach

Create custom clang-tidy check to verify all `widget_container` subclasses call `set_layout_strategy()` in constructor:

**.clang-tidy:**
```yaml
Checks: '-*,
  custom-require-layout-strategy'

CheckOptions:
  - key: custom-require-layout-strategy.ContainerBaseClass
    value: 'widget_container'
  - key: custom-require-layout-strategy.LayoutMethod
    value: 'set_layout_strategy'
```

**Custom check implementation:**
```cpp
// clang-tidy/RequireLayoutStrategyCheck.cpp
class RequireLayoutStrategyCheck : public ClangTidyCheck {
public:
  void registerMatchers(MatchFinder *Finder) override {
    // Match all classes inheriting from widget_container
    Finder->addMatcher(
      cxxRecordDecl(
        isDerivedFrom("widget_container")
      ).bind("container"),
      this
    );
  }

  void check(const MatchFinder::MatchResult &Result) override {
    auto *Container = Result.Nodes.getNodeAs<CXXRecordDecl>("container");

    // Check all constructors
    for (auto *Ctor : Container->ctors()) {
      if (!callsSetLayoutStrategy(Ctor)) {
        diag(Ctor->getLocation(),
          "Container must call set_layout_strategy() in constructor")
          << FixItHint::CreateInsertion(/* suggest fix */);
      }
    }
  }
};
```

### How It Works

Run clang-tidy during CI/CD:
```bash
cmake -DONYX_UI_ENABLE_CLANG_TIDY=ON
cmake --build .  # Fails if rule violated
```

**Output on violation:**
```
window.cc:25: error: Container 'window' must call set_layout_strategy() in constructor [custom-require-layout-strategy]
```

### Pros
✅ **Compile-time enforcement** (via static analysis)
✅ **No breaking changes** to existing code
✅ Works with CI/CD pipelines
✅ Can provide fix-it hints
✅ Extensible to other requirements

### Cons
❌ Requires clang-tidy infrastructure
❌ Only catches bugs when clang-tidy is enabled
❌ More complex to implement custom check
❌ Developers might disable locally and forget

### Effort: MEDIUM (4-6 hours to implement custom check)

---

## Option 3: C++20 Concept (Type-Level Constraint)

### Approach

Create concept requiring containers to have layout strategy:

```cpp
template<typename T, UIBackend Backend>
concept HasLayoutStrategy = requires(T container) {
    { container.get_layout_strategy() } -> std::convertible_to<layout_strategy<Backend>*>;
};

template<UIBackend Backend>
class widget_container : public widget<Backend> {
protected:
    widget_container(ui_element<Backend>* parent = nullptr)
        : base(parent)
    {
        // Concept check enforced at instantiation
        static_assert(
            HasLayoutStrategy<decltype(*this), Backend>,
            "Container must set layout_strategy in constructor"
        );
    }
};
```

### How It Works

Concept checked when container is instantiated:

```cpp
window<test_backend> win;  // Compile error if no layout strategy
```

**Compiler error:**
```
error: static assertion failed: Container must set layout_strategy in constructor
note: concept 'HasLayoutStrategy<window<test_backend>, test_backend>' was not satisfied
```

### Pros
✅ Modern C++20 approach
✅ Clear conceptual model
✅ Works with template metaprogramming

### Cons
❌ **Runtime check masquerading as compile-time** (static_assert in constructor still runs at runtime)
❌ Doesn't actually prevent construction without layout
❌ Concept can't verify constructor behavior
❌ Weaker than deleted constructor approach

### Effort: LOW (1 hour to implement)
### Effectiveness: ⚠️ WEAK (false sense of security)

---

## Option 4: Debug Assertions (Runtime - What We Have Now)

### Current Approach

Add assertions in `do_measure()`:

```cpp
template<UIBackend Backend>
size_type ui_element<Backend>::do_measure(int available_width, int available_height) {
    if (m_layout_strategy) {
        return m_layout_strategy->measure_children(...);
    }

    #ifdef DEBUG
    // RUNTIME assertion - catches bug during development
    if (!children().empty()) {
        assert(false && "Container has children but no layout_strategy!");
    }
    #endif

    return get_content_size();  // Returns {0,0}
}
```

### Pros
✅ Easy to implement
✅ **No breaking changes**
✅ Catches bug during development/testing
✅ Provides clear error message

### Cons
❌ **Runtime check** (not compile-time)
❌ Only fires if container has children
❌ Disabled in release builds
❌ Requires running code to catch bug

### Effort: MINIMAL (30 minutes)
### Effectiveness: MEDIUM (catches bugs in debug builds only)

---

## Option 5: Visual Tests (What We Implemented)

### Current Approach

Mandatory `render_to_canvas()` test for every container:

```cpp
SUBCASE("Visual rendering - layout strategy verification") {
    my_container<test_canvas_backend> container;
    container.add_child(...);

    auto size = container.measure(100, 50);
    CHECK(size.w > 0);  // FAILS if no layout strategy
    CHECK(size.h > 0);
}
```

### Pros
✅ **Already implemented** (100% coverage)
✅ **No breaking changes**
✅ Catches ALL rendering bugs (not just layout)
✅ Tests complete pipeline
✅ Works with any CI/CD

### Cons
❌ **Runtime check** (not compile-time)
❌ Requires running tests
❌ Bug found after code is written

### Effort: COMPLETE (already done)
### Effectiveness: HIGH (catches bug reliably in CI/CD)

---

## Recommendation: Multi-Layered Defense

Combine multiple approaches for defense in depth:

### Tier 1: Visual Tests ✅ (DONE)
- **Already implemented** - 100% container coverage
- Catches bugs in CI/CD before merge
- **Keep this as ultimate safety net**

### Tier 2: Debug Assertions ⭐ (RECOMMENDED)
- Add runtime assertions in `do_measure()`
- Catches bugs during local development
- **Minimal effort, high value**

### Tier 3: Clang-Tidy Rule (OPTIONAL)
- Custom static analyzer check
- Catches bugs during build
- **Medium effort, good for large teams**

### Tier 4: Deleted Constructor (FUTURE)
- Strongest compile-time guarantee
- Requires refactoring all containers
- **High effort, but bulletproof**

---

## Immediate Action Items

**Phase 1 (Today): Debug Assertions**
1. Add assertion in `ui_element::do_measure()`
2. Test with containers missing layout strategy
3. Verify clear error message

**Phase 2 (This Week): Clang-Tidy Rule**
1. Implement custom `require-layout-strategy` check
2. Add to CI/CD pipeline
3. Document in CONTRIBUTING.md

**Phase 3 (Future Refactor): Deleted Constructor**
1. Create migration plan for all containers
2. Update containers incrementally
3. Delete old constructor once complete

---

## Comparison Matrix

| Approach | Timing | Strength | Breaking | Effort |
|----------|--------|----------|----------|--------|
| **Deleted Constructor** | Compile | ⭐⭐⭐⭐⭐ | ❌ YES | HIGH |
| **Clang-Tidy** | Compile | ⭐⭐⭐⭐ | ✅ NO | MEDIUM |
| **C++20 Concept** | Compile* | ⭐⭐ | ✅ NO | LOW |
| **Debug Assertions** | Runtime | ⭐⭐⭐ | ✅ NO | MINIMAL |
| **Visual Tests** | Runtime | ⭐⭐⭐⭐ | ✅ NO | COMPLETE |

*Concept is compile-time checked but doesn't prevent construction

---

## Conclusion

**For immediate protection:** Implement **Debug Assertions** (30 minutes)

**For compile-time enforcement:** Implement **Clang-Tidy Rule** (4-6 hours)

**For ultimate safety:** Keep **Visual Tests** at 100% (already complete)

All three layers together provide defense in depth:
1. 🔴 **Compile-time:** Clang-tidy catches before build
2. 🟡 **Development-time:** Assertions catch during testing
3. 🟢 **CI/CD-time:** Visual tests catch before merge
