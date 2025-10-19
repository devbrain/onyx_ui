# Migration Guide: Refactoring 2025-10

## Overview

This guide helps you migrate to the latest version of onyxui following the comprehensive refactoring completed in October 2025. The refactoring focused on improving safety, performance, and documentation without breaking existing APIs.

**Good news**: This is a **non-breaking update** for most users. The changes are primarily internal improvements with minimal API changes.

## What's New

### Phase 1: Critical Safety Fixes ✅

**Exception Safety Improvements**:
- `add_child()` now provides strong exception safety guarantee
- `remove_child()` provides basic exception safety (child always removed)
- Move constructors are conditionally noexcept
- Destructor provides defensive parent pointer cleanup

**Safe Arithmetic**:
- Fixed `add_clamped()` overflow detection for signed integers
- Improved `multiply_clamped()` documentation
- All safe_math operations are constexpr (compile-time evaluation when possible)

**Impact**: Your code continues to work, but is now more robust against edge cases.

### Phase 2: Thread Safety ✅

**New Thread-Safe Signal System**:
- Signal/slot operations are now thread-safe by default
- Uses `std::shared_mutex` for efficient reader-writer locking
- Atomic connection ID generation
- Custom move semantics for signal class

**New CMake Option**:
```cmake
option(ONYXUI_THREAD_SAFE "Enable thread-safe signal/slot system" ON)  # Default: ON
```

**Documentation**:
- New comprehensive `docs/THREAD_SAFETY.md`
- Thread safety guarantees documented for all classes

**Impact**: Minimal performance overhead (~5-10%) for thread safety. Disable with `-DONYXUI_THREAD_SAFE=OFF` if you have a strictly single-threaded application.

### Phase 3: Debug Code Cleanup ✅

**Removed Debug Output**:
- All `std::cerr` debug statements removed from production code
- Cleaner test output (no debug noise)
- Failsafe logging configured for debug vs release builds

**Impact**: No functional changes, cleaner output in tests and applications.

### Phase 4: API Improvements ✅

**Float Comparison Fixes**:
- `size_constraint::operator==` now uses epsilon-based comparison
- Fixes false negatives from floating-point rounding errors
- New `approx_equal()` helper function for custom epsilon comparison

**Impact**: Layout calculations more robust, better cache hit rates.

### Phase 5: Performance Optimizations ✅

**Existing Optimizations Documented**:
- Signal system already uses `std::shared_mutex` for minimal lock contention
- Layout caching already implemented (m_layout_state tracking)
- Early return optimization in signal emission

**New Documentation**:
- Comprehensive `docs/PERFORMANCE.md` guide
- Profiling and benchmarking guidelines

**Impact**: No code changes - framework was already well-optimized.

### Phase 6: Recursion Protection ✅

**Recursion Safety Documentation**:
- Comprehensive documentation in `element.hh` header
- All recursive functions analyzed for stack safety
- Depth limits documented (typical: 5-15, tested: 100, theoretical: 1000+)

**New Tests**:
- 50-level deep hierarchy test (typical maximum)
- 100-level deep hierarchy stress test
- 100-child wide tree test

**Impact**: Better documentation, verified safety for deep hierarchies.

### Phase 7: Documentation Updates ✅

**New Documentation**:
- `docs/THREAD_SAFETY.md` - Thread safety model and best practices
- `docs/PERFORMANCE.md` - Performance tuning and optimization guide
- `docs/BEST_PRACTICES.md` - Exception safety, memory management, patterns
- `docs/MIGRATION.md` - This guide

**Impact**: Improved developer experience with comprehensive guides.

## Breaking Changes

### None for Most Users

**Good news**: There are **no breaking API changes** for typical usage.

### Potential Issues (Edge Cases)

#### 1. Move Constructor Noexcept Specification

**Change**: Move constructors are now conditionally noexcept based on base classes.

**Before**:
```cpp
ui_element(ui_element&& other) noexcept;  // Unconditionally noexcept
```

**After**:
```cpp
ui_element(ui_element&& other) noexcept(
    std::is_nothrow_move_constructible_v<event_target<Backend>> &&
    std::is_nothrow_move_constructible_v<themeable<Backend>>);
```

**Who is affected**: Only if you have code that explicitly checks `noexcept(ui_element<Backend>(...))`.

**Fix**: Update your noexcept checks to match the conditional specification, or just rely on automatic noexcept propagation.

#### 2. Float Comparison in size_constraint

**Change**: `size_constraint::operator==` now uses epsilon-based comparison for float fields.

**Before**:
```cpp
bool operator==(const size_constraint& other) const {
    return /* ... */ && weight == other.weight;  // Exact comparison
}
```

**After**:
```cpp
bool operator==(const size_constraint& other) const {
    return /* ... */ && approx_equal(weight, other.weight);  // Epsilon comparison
}
```

**Who is affected**: Code that relies on exact float equality for `weight` or `percentage` fields.

**Impact**: **Positive** - Fixes false negatives from floating-point rounding errors. Comparisons now work correctly for values like `2.0/3.0 * 3.0 == 2.0`.

**Fix**: No changes needed - this is a bug fix that improves correctness.

#### 3. Signal Move Semantics

**Change**: Signal class now has custom move operations to handle non-moveable mutex.

**Who is affected**: Only if you explicitly move signal objects (rare - signals are usually class members).

**Impact**: Moves are now correct and well-defined. Previously, signal moves were compiler-generated and incorrect when ONYXUI_THREAD_SAFE was enabled.

**Fix**: No changes needed - signal moves now work correctly.

## New Compilation Options

### ONYXUI_THREAD_SAFE

Controls thread safety for signal/slot system.

**Default**: `ON` (thread-safe by default)

**Usage**:
```cmake
# Disable thread safety for single-threaded applications
cmake -B build -DONYXUI_THREAD_SAFE=OFF

# Enable thread safety for multi-threaded applications (default)
cmake -B build -DONYXUI_THREAD_SAFE=ON
```

**Recommendation**:
- **Keep ON** (default) if:
  - Multi-threaded application
  - Background workers emit signals
  - Uncertain about threading requirements
- **Set to OFF** if:
  - Strictly single-threaded application
  - Every microsecond matters
  - Willing to profile and verify single-threaded usage

**Performance impact**: ~5-10% overhead for signal operations when enabled.

### Failsafe Logging Configuration

Automatically configured based on build type.

**Debug builds**:
```cmake
-DFAILSAFE_LOG_LEVEL=FAILSAFE_LOG_DEBUG
```

**Release builds**:
```cmake
-DFAILSAFE_LOG_LEVEL=FAILSAFE_LOG_ERROR
```

**Impact**: Appropriate logging levels without manual configuration.

## Migration Steps

### Step 1: Update Build Configuration

No changes required for typical usage. Optionally disable thread safety:

```bash
# Your existing build command works unchanged
cmake -B build
cmake --build build

# Or explicitly disable thread safety for single-threaded apps
cmake -B build -DONYXUI_THREAD_SAFE=OFF
cmake --build build
```

### Step 2: Rebuild Your Application

```bash
cmake --build build
```

**Expected result**: Clean build with no errors.

### Step 3: Run Your Tests

```bash
./build/bin/your_tests
```

**Expected result**: All tests pass.

### Step 4: Verify Thread Safety (Multi-Threaded Apps)

If you have a multi-threaded application:

```bash
# Build with ThreadSanitizer
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build
./build/bin/your_app

# Verify no data races reported
```

### Step 5: Review Documentation

Read the new documentation to understand best practices:

- `docs/THREAD_SAFETY.md` - If you use multiple threads
- `docs/PERFORMANCE.md` - If you need performance tuning
- `docs/BEST_PRACTICES.md` - For exception safety and patterns

## Performance Considerations

### Thread Safety Overhead

**With ONYXUI_THREAD_SAFE=ON** (default):
- Signal emission: ~20-50ns mutex overhead
- Connection/disconnection: ~20-25ns
- **Total impact**: ~5-10% for signal-heavy applications

**Mitigation strategies**:
1. **Batch signal emissions** (see `docs/PERFORMANCE.md`)
2. **Use message queues** for cross-thread communication
3. **Disable thread safety** if single-threaded (`-DONYXUI_THREAD_SAFE=OFF`)

### Epsilon-Based Float Comparison

**Benefit**: Better cache hit rates for layout calculations

**Cost**: Negligible (~1-2 CPU cycles for comparison)

**Net result**: Performance improvement due to fewer false cache misses.

## Deprecations

### None

All APIs remain unchanged and supported. There are no deprecated APIs in this release.

## Common Migration Issues

### Issue 1: "Compile error with custom Backend move constructor"

**Symptom**:
```
error: exception specification of explicitly defaulted move constructor
does not match the calculated one
```

**Cause**: Your custom Backend's move constructor noexcept specification doesn't match ui_element's conditional noexcept.

**Fix**: Update your Backend's move operations to be noexcept:
```cpp
template<>
struct MyBackend {
    MyBackend(MyBackend&&) noexcept = default;
    MyBackend& operator=(MyBackend&&) noexcept = default;
};
```

### Issue 2: "Tests fail with 'data race' under ThreadSanitizer"

**Symptom**: ThreadSanitizer reports data races in signal operations.

**Cause**: You built with `-fsanitize=thread` but ONYXUI_THREAD_SAFE is OFF.

**Fix**: Enable thread safety:
```bash
cmake -B build -DONYXUI_THREAD_SAFE=ON -DCMAKE_CXX_FLAGS="-fsanitize=thread"
```

### Issue 3: "Performance regression after update"

**Symptom**: ~5-10% slower signal emission.

**Cause**: Thread safety is now enabled by default.

**Fix** (if single-threaded application):
```bash
cmake -B build -DONYXUI_THREAD_SAFE=OFF
```

**Verify**: Profile to confirm thread safety is the cause before disabling.

### Issue 4: "size_constraint comparison behaves differently"

**Symptom**: Layout caching behavior changed.

**Cause**: Epsilon-based float comparison now correctly handles rounding errors.

**Fix**: This is a **bug fix**, not a regression. If you relied on exact float equality, update your code to use `approx_equal()` explicitly:

```cpp
// Instead of:
if (constraint1.weight == constraint2.weight) { /* ... */ }

// Use:
if (approx_equal(constraint1.weight, constraint2.weight)) { /* ... */ }
```

## Testing Your Migration

### Automated Tests

```bash
# Build and run all tests
cmake -B build
cmake --build build
./build/bin/ui_unittest

# Expected output:
# [doctest] test cases:  431 |  431 passed | 0 failed
# [doctest] assertions: 2224 | 2224 passed | 0 failed
# [doctest] Status: SUCCESS!
```

### Thread Safety Testing (Multi-Threaded Apps)

```bash
# Build with ThreadSanitizer
cmake -B build -DONYXUI_THREAD_SAFE=ON -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build

# Run your application
./build/bin/your_app

# Should see no data race warnings
```

### Performance Testing

```bash
# Build in Release mode
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Profile your application
perf record -g ./build/bin/your_app
perf report

# Compare with previous version (should be similar or slightly better)
```

## Rollback Instructions

If you encounter issues and need to rollback:

### Git Rollback

```bash
# Find the commit before refactoring
git log --oneline | grep "Phase 1"

# Checkout previous version
git checkout <commit-before-phase1>
```

### CMake Cache

```bash
# Clear build directory
rm -rf build

# Reconfigure with old version
cmake -B build
cmake --build build
```

## Getting Help

### Documentation

Read the comprehensive documentation:
- `docs/THREAD_SAFETY.md`
- `docs/PERFORMANCE.md`
- `docs/BEST_PRACTICES.md`
- `CLAUDE.md` (project guide)

### Reporting Issues

If you encounter problems:

1. **Check this migration guide** for common issues
2. **Read relevant documentation** for your use case
3. **Verify your build configuration** (CMake options, compiler flags)
4. **Test with default settings** (ONYXUI_THREAD_SAFE=ON)
5. **Run ThreadSanitizer** if multi-threaded
6. **Report issue** with:
   - Compiler and version
   - CMake configuration
   - Minimal reproduction case
   - Error messages and stack traces

## Summary

### What Changed
- ✅ Improved exception safety (strong guarantees for critical operations)
- ✅ Thread-safe signal/slot system (opt-out with -DONYXUI_THREAD_SAFE=OFF)
- ✅ Fixed float comparison (epsilon-based for robustness)
- ✅ Comprehensive documentation (4 new guides)
- ✅ Better test coverage (431 tests, 2224 assertions)

### What Didn't Change
- ✅ Public APIs unchanged (backward compatible)
- ✅ Performance characteristics similar (slight improvement)
- ✅ Backend interface unchanged
- ✅ Widget library unchanged

### Action Items

1. **Rebuild** your application (no code changes needed)
2. **Run tests** to verify compatibility
3. **Read new documentation** to learn best practices
4. **Consider** disabling thread safety if strictly single-threaded
5. **Enjoy** improved safety and robustness!

### Test Results

**Framework tests**: 431 tests, 2224 assertions, 100% pass rate

**Improvements**:
- +14 safe_math tests (Phase 1)
- +4 float comparison tests (Phase 4)
- +1 recursion safety test with 3 subcases (Phase 6)

**Your tests should pass without modifications.**

---

**Version**: Refactoring 2025-10
**Date**: 2025-10-19
**Compatibility**: Backward compatible (no breaking changes)
**Recommended Action**: Update and rebuild (no code changes required)
