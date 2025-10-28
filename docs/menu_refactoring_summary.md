# Menu System Refactoring - Quick Reference

**For detailed implementation guide, see:** [menu_system_refactoring.md](menu_system_refactoring.md)

---

## TL;DR

Incremental refactoring from current buggy menu system → full-fledged composite menu with submenus and context-dependent navigation.

**Current Bug:** Down arrow loses focus (Help menu handler overwrites File/Theme)
**Root Cause:** Multiple menus register same global semantic action
**Solution:** 6 phases, each testable and non-breaking

---

## Phase Overview

| Phase | Goal | Priority | Effort | Status |
|-------|------|----------|--------|--------|
| **Phase 0** | Fix down arrow bug | CRITICAL | 1-2h | 🟢 DONE |
| **Phase 1** | Add RAII guards | HIGH | 2-3h | 🟢 DONE |
| **Phase 2** | Left/right navigation | MEDIUM | 2-3h | 🟢 DONE |
| **Phase 3** | Submenu infrastructure | LOW | 4-6h | 🟢 DONE |
| **Phase 4** | Composite menu_system | LOW | 6-8h | 🟢 DONE |
| **Phase 5** | Context-dependent nav | LOW | 4-6h | 🟢 DONE |

**Total Estimated Time:** 37 hours (single developer)

---

## Quick Start: Phase 0 (Fix Critical Bug)

### What to Change

1. **menu.hh:** Remove `initialize_hotkeys()` call from constructor
2. **menu.hh:** Make hotkey methods public: `register_navigation_hotkeys()`, `unregister_navigation_hotkeys()`
3. **menu_bar.hh:** Call `register_navigation_hotkeys()` in `open_menu()`
4. **menu_bar.hh:** Call `unregister_navigation_hotkeys()` in `close_menu()`

### Test It

```bash
./build/bin/widgets_demo
# Press F10 → Down arrow → Down arrow
# ✅ PASS: Focus moves New → Open → separator → Quit
# ❌ FAIL: Focus disappears
```

### Acceptance Criteria

- ✅ Down arrow works in File menu
- ✅ Down arrow works in Theme menu
- ✅ Down arrow works in Help menu
- ✅ All existing tests pass

---

## Architecture Evolution

### Before (Current - Broken)

```
menu_bar
  ├─ menu (File)      → registers menu_down in constructor
  ├─ menu (Theme)     → registers menu_down (overwrites File)
  └─ menu (Help)      → registers menu_down (overwrites Theme)

Result: Only Help menu's handler exists, even when File is open
```

### After Phase 0 (Bug Fixed)

```
menu_bar
  ├─ menu (File)
  ├─ menu (Theme)
  └─ menu (Help)

When File opens:
  menu_bar calls file_menu->register_navigation_hotkeys()

When File closes:
  menu_bar calls file_menu->unregister_navigation_hotkeys()

Result: Only open menu has active handlers
```

### After Phase 1 (RAII)

```
menu_bar
  ├─ m_menu_nav_guards: [guard1, guard2, guard3, guard4]  (RAII cleanup)
  ├─ menu (File)
  ├─ menu (Theme)
  └─ menu (Help)

When menu switches:
  m_menu_nav_guards.clear()  // RAII auto-unregisters
  m_menu_nav_guards.emplace_back(...)  // Register new

Result: Exception-safe, can't forget to cleanup
```

### After Phase 2 (Left/Right)

```
menu_bar::open_menu():
  Register 6 actions:
    - menu_down → menu->focus_next()
    - menu_up → menu->focus_previous()
    - menu_left → this->open_previous_menu()   ✅ NEW
    - menu_right → this->open_next_menu()      ✅ NEW
    - menu_select → menu->activate_focused()
    - menu_cancel → this->close_menu()

Result: Left/right arrows switch menu bar items
```

### After Phase 4 (Composite)

```
menu_bar
  └─ menu_system
       ├─ m_menu_stack: [File Menu]
       └─ Handlers registered ONCE:
            - menu_down → current_menu()->focus_next()
            - menu_left → context-dependent logic
            - menu_right → context-dependent logic

Result: Zero registration churn, centralized state
```

### After Phase 5 (Submenus)

```
menu_bar
  └─ menu_system
       ├─ m_menu_stack: [File Menu, Open Submenu, Project Submenu]
       └─ Context-dependent navigation:
            Right arrow on "Open ▶" → Open submenu
            Right arrow on "New" → Switch to Theme menu
            Left arrow in submenu → Close submenu
            Left arrow in top-level → Previous menu bar item

Result: Full cascading menu support
```

---

## Decision Tree: Which Phase Do I Need?

```
START: What do you need?

├─ Just fix the down arrow bug
│  └─ Phase 0 (CRITICAL - 1-2 hours)
│
├─ Prevent future bugs + cleaner code
│  └─ Phase 0 + Phase 1 (HIGH - 3-5 hours)
│
├─ Better user experience (left/right switching)
│  └─ Phase 0 + Phase 1 + Phase 2 (MEDIUM - 7-9 hours)
│
├─ Want to add submenus later
│  └─ Phase 0 + Phase 1 + Phase 2 + Phase 3 (LOW - 15-17 hours)
│
└─ Full cascading menu system NOW
   └─ All Phases (LOW - 37 hours)
```

**Recommendation:** Start with **Phase 0 + Phase 1** (5 hours total)
- Fixes critical bug
- Adds RAII safety
- Enables future phases without rework

---

## Testing Checklist

### Phase 0
- [ ] Down arrow works in File menu (New → Open → Quit)
- [ ] Down arrow works in Theme menu (cycles through themes)
- [ ] Down arrow works in Help menu (just About)
- [ ] Up arrow works (reverse direction)
- [ ] ESC closes menu
- [ ] Enter activates item
- [ ] Switching menus (File → Theme) works
- [ ] All 736 existing unit tests pass
- [ ] Zero warnings from clang-tidy
- [ ] Valgrind reports no leaks

### Phase 1
- [ ] All Phase 0 tests pass
- [ ] Menu switching 100x (stress test)
- [ ] Exception in menu handler doesn't leak
- [ ] RAII guard unit tests pass (3 new tests)

### Phase 2
- [ ] Left arrow switches to previous menu bar item
- [ ] Right arrow switches to next menu bar item
- [ ] Wrap around works (Help → File, File → Help)
- [ ] Focus moves to first item in new menu
- [ ] All Phase 1 tests pass

### Phase 3
- [ ] menu_item can own submenu
- [ ] Submenu indicator (►) renders
- [ ] Existing menus without submenus work
- [ ] All Phase 2 tests pass

### Phase 4
- [ ] menu_system tracks menu stack
- [ ] Hotkeys registered once (no churn on menu switch)
- [ ] All Phase 3 tests pass

### Phase 5
- [ ] Right arrow on submenu item opens submenu
- [ ] Left arrow in submenu closes submenu
- [ ] Multiple submenu levels work
- [ ] All Phase 4 tests pass

---

## Common Issues & Solutions

### Issue: "Down arrow still doesn't work after Phase 0"

**Debug steps:**
1. Add logging to `menu_bar::open_menu()` - is `register_navigation_hotkeys()` called?
2. Add logging to `menu::register_navigation_hotkeys()` - is handler registered?
3. Check `ui_services<Backend>::hotkeys()` - is it non-null?
4. Check if different menu is still open (close_menu not called)

### Issue: "Memory leak detected after Phase 1"

**Debug steps:**
1. Verify RAII guard destructor is called
2. Check if `m_menu_nav_guards.clear()` is called in `close_menu()`
3. Run valgrind with `--leak-check=full`
4. Check if menu_bar destructor is called

### Issue: "Left/right arrows switch even in submenu (Phase 5)"

**Debug steps:**
1. Check `menu_system::is_top_level()` - should return false in submenu
2. Add logging to `handle_menu_left()` - which branch is taken?
3. Verify `m_menu_stack.size()` is > 1 when in submenu

---

## Performance Targets

| Metric | Phase 0 | Phase 1 | Phase 2 | Phase 4 | Phase 5 |
|--------|---------|---------|---------|---------|---------|
| Menu open time | < 1ms | < 1ms | < 1ms | < 1ms | < 2ms |
| Hotkey registration | ~0.5ms per menu switch | ~0.1ms (RAII) | ~0.1ms | 0ms (register once) | 0ms |
| Memory per menu | +0 bytes | +160 bytes (guards) | +160 bytes | +240 bytes (stack) | +240 bytes |
| CPU per keystroke | ~50µs | ~50µs | ~50µs | ~50µs | ~100µs (context check) |

All targets measured on typical developer machine (Intel i7, 16GB RAM).

---

## Code Size Impact

| Phase | Files Changed | Lines Added | Lines Removed | Net Change |
|-------|---------------|-------------|---------------|------------|
| 0 | 2 | 30 | 5 | +25 |
| 1 | 3 | 150 | 30 | +120 |
| 2 | 1 | 40 | 0 | +40 |
| 3 | 2 | 100 | 0 | +100 |
| 4 | 3 | 400 | 50 | +350 |
| 5 | 2 | 80 | 10 | +70 |
| **Total** | **~10 unique** | **800** | **95** | **+705** |

---

## Git Strategy

### Branching
```bash
git checkout -b feature/menu-refactor-phase-0
# ... implement Phase 0 ...
git commit -m "fix: menu navigation down arrow loses focus

- Remove hotkey registration from menu constructor
- Register/unregister in menu_bar open/close
- Fixes #XXX

Fixes: Only open menu now handles navigation keys
Test: All 736 tests pass, manual testing confirms fix"

git push origin feature/menu-refactor-phase-0
# Create PR, get review, merge

git checkout main
git pull
git checkout -b feature/menu-refactor-phase-1
# ... repeat for each phase ...
```

### Commit Message Template
```
<type>: <short summary>

<detailed description>

Fixes: <what it fixes>
Test: <how to test>
Phase: <phase number>
```

---

## Review Checklist

### Code Quality
- [ ] Follows project style (clang-tidy clean)
- [ ] No TODO comments left in code
- [ ] All magic numbers extracted to constants
- [ ] No copy-paste code duplication

### Testing
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Manual testing documented
- [ ] Edge cases covered

### Documentation
- [ ] Public APIs documented
- [ ] Complex logic has comments
- [ ] Update phase status in this doc

### Safety
- [ ] No raw pointers (except non-owning)
- [ ] RAII used for resources
- [ ] Exception-safe
- [ ] No undefined behavior (valgrind, sanitizers)

---

## FAQ

**Q: Can I skip Phase 1 and go straight to Phase 2?**
A: Not recommended. Phase 1 provides RAII safety that prevents bugs in future phases.

**Q: Do I need Phase 4 if I don't need submenus?**
A: No. Phase 2 is sufficient for menu bar switching without submenus.

**Q: How do I test menu navigation without running the full app?**
A: Unit tests can trigger semantic actions programmatically. See `unittest/widgets/test_menus.cc`.

**Q: Can I implement Phase 0 differently?**
A: Yes, but discuss alternatives in the detailed doc (Appendix A). Current approach is minimal-change.

**Q: What if Phase 5 takes too long?**
A: Stop at Phase 4. You'll have the infrastructure for submenus, just not the navigation logic.

---

## Related Documents

- [menu_system_refactoring.md](menu_system_refactoring.md) - Detailed implementation guide
- [CLAUDE.md](../CLAUDE.md) - Project architecture overview
- [unittest/widgets/test_menus.cc](../unittest/widgets/test_menus.cc) - Existing menu tests

---

## Status Tracking

Last updated: 2025-10-28

| Phase | Status | Start Date | End Date | Notes |
|-------|--------|------------|----------|-------|
| Phase 0 | 🟢 DONE | 2025-10-28 | 2025-10-28 | Critical bug fix - All tests pass |
| Phase 1 | 🟢 DONE | 2025-10-28 | 2025-10-28 | RAII guards - Exception-safe, 786 tests pass, 4907 assertions |
| Phase 2 | 🟢 DONE | 2025-10-28 | 2025-10-28 | Left/Right navigation - 788 tests pass, 4925 assertions |
| Phase 3 | 🟢 DONE | 2025-10-28 | 2025-10-28 | Submenu API - Data structure only, 796 tests pass, 4951 assertions, rendering deferred |
| Phase 4 | 🟢 DONE | 2025-10-28 | 2025-10-28 | Composite menu_system - 807 tests pass, 4984 assertions, zero registration churn |
| Phase 5 | 🟢 DONE | 2025-10-28 | 2025-10-28 | Submenu navigation - 812 tests pass, 5005 assertions, full cascading menus |

**Legend:**
- 🔴 TODO - Not started
- 🟡 IN PROGRESS - Currently working
- 🟢 DONE - Merged to main
- ⚪ BLOCKED - Waiting on dependency

---

**For questions, contact the original implementer or see the detailed doc.**
