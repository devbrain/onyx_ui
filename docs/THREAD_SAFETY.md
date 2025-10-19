# Thread Safety Model

## Overview

The onyxui framework provides different thread safety guarantees for different components. This document outlines the threading model, thread-safe components, and best practices for multi-threaded usage.

## Thread-Safe Components

### Signal/Slot System (with ONYXUI_THREAD_SAFE enabled)

The signal/slot system is fully thread-safe when compiled with `ONYXUI_THREAD_SAFE=ON`:

- **Thread-safe operations**:
  - `signal::connect()` - Can be called from any thread
  - `signal::disconnect()` - Can be called from any thread
  - `signal::emit()` - Can be called from any thread
  - `scoped_connection` - Destructor is thread-safe

- **Implementation**:
  - Uses `std::shared_mutex` for reader-writer locking
  - Connection ID generation is atomic (`std::atomic<connection_id>`)
  - Callbacks are invoked without holding locks to prevent deadlock
  - Slots are copied before emission to allow safe disconnect during callback

**Example**:
```cpp
signal<int, std::string> data_changed;

// Thread 1: Connect
std::thread t1([&]() {
    data_changed.connect([](int id, const std::string& name) {
        std::cout << "Data " << id << " changed to " << name << "\n";
    });
});

// Thread 2: Emit
std::thread t2([&]() {
    data_changed.emit(42, "value");
});

t1.join();
t2.join();
```

### scoped_connection (with ONYXUI_THREAD_SAFE enabled)

- **Thread-safe disconnect**: Destructor and manual `disconnect()` can be called from any thread
- **Implementation**: Uses `std::mutex` to protect connection state
- **Caveat**: The signal must outlive the scoped_connection

## Single-Threaded Components (UI Thread Only)

The following components are **NOT thread-safe** and must only be accessed from the main UI thread:

### focus_manager

- **Restriction**: All operations must occur on the UI thread
- **Reason**: Manages UI focus state which is inherently single-threaded
- **Workaround**: Use message passing to queue focus changes from other threads

### Layout System (measure/arrange)

- **Restriction**: All layout operations must occur on the UI thread
- **Components affected**:
  - `ui_element::measure()`
  - `ui_element::arrange()`
  - `ui_element::invalidate_measure()`
  - `ui_element::invalidate_arrange()`
- **Reason**: Layout state and caching are not synchronized
- **Workaround**: Queue layout invalidations for processing on UI thread

### Event Routing

- **Restriction**: Event handling must occur on the UI thread
- **Components affected**:
  - `event_target::on_mouse_down()`, `on_mouse_up()`, etc.
  - `ui_element::hit_test()`
- **Reason**: Event state and focused element tracking are not synchronized

### Element Tree Modifications

- **Restriction**: All tree modifications must occur on the UI thread
- **Operations**:
  - `add_child()`
  - `remove_child()`
  - `clear_children()`
  - Destructor
- **Reason**: Parent pointers and child vectors are not synchronized

## Thread-Safe with Caveats

### grid_layout

- **Status**: Single-threaded (UI thread only)
- **Mutable cache**: `m_column_widths` and `m_row_heights` are not synchronized
- **Restriction**: All grid layout operations must occur on the UI thread
- **Reason**: Mutable state used during const operations (logical constness)
- **Future**: Could be made thread-safe with mutex or thread-local storage if needed

### layer_manager

- **Event routing**: Uses signals internally (thread-safe with ONYXUI_THREAD_SAFE)
- **Layer modifications**: Must occur on UI thread
- **Safe operations**: Receiving events from background threads (via signals)
- **Unsafe operations**: Adding/removing layers from background threads

## Compilation Flags

### ONYXUI_THREAD_SAFE

Controls whether thread safety is enabled:

```cmake
# Enable thread-safe signal/slot system (default: ON)
cmake -B build -DONYXUI_THREAD_SAFE=ON

# Disable for maximum performance in single-threaded apps
cmake -B build -DONYXUI_THREAD_SAFE=OFF
```

**When enabled** (default):
- Signal/slot uses `std::shared_mutex`
- `scoped_connection` uses `std::mutex`
- Grid layout uses thread-local cache
- Small performance overhead (~5-10%)

**When disabled**:
- No mutex overhead
- Maximum performance
- **Only safe for single-threaded usage**

## Guidelines

### 1. Signal Connections/Disconnections

✅ **Safe**: Connect/disconnect from any thread (with ONYXUI_THREAD_SAFE)
```cpp
// Background thread
std::thread worker([&signal]() {
    auto conn = signal.connect([]() { /* handler */ });
    // conn destructor automatically disconnects
});
```

❌ **Unsafe**: Assuming immediate emission visibility without synchronization
```cpp
// Thread 1
signal.connect(handler);

// Thread 2 (immediately after)
signal.emit();  // Handler may or may not be connected yet!
```

### 2. Signal Emissions

✅ **Safe**: Emit from any thread (with ONYXUI_THREAD_SAFE)
```cpp
signal<int> value_changed;

// Background thread
std::thread worker([&]() {
    int result = compute();
    value_changed.emit(result);  // Safe
});
```

⚠️ **Caveat**: Callbacks execute synchronously on the emitting thread
```cpp
value_changed.connect([](int val) {
    // This runs on the worker thread!
    // Don't call UI functions here
});
```

### 3. Cross-Thread UI Updates

✅ **Correct**: Use message queue for UI thread
```cpp
class Application {
    std::queue<std::function<void()>> m_ui_tasks;
    std::mutex m_ui_mutex;

    void process_ui_tasks() {
        std::lock_guard lock(m_ui_mutex);
        while (!m_ui_tasks.empty()) {
            m_ui_tasks.front()();
            m_ui_tasks.pop();
        }
    }

public:
    void queue_ui_task(std::function<void()> task) {
        std::lock_guard lock(m_ui_mutex);
        m_ui_tasks.push(std::move(task));
    }
};

// Background thread
std::thread worker([&app, &button]() {
    int result = compute();
    app.queue_ui_task([&button, result]() {
        button->set_text(std::to_string(result));  // Safe on UI thread
    });
});
```

❌ **Incorrect**: Direct UI modification from background thread
```cpp
std::thread worker([&button]() {
    button->set_text("Done");  // UNSAFE! Not on UI thread
});
```

### 4. Layout Calculations

❌ **Incorrect**: Measure/arrange from background thread
```cpp
std::thread worker([&element]() {
    element->measure(100, 100);  // UNSAFE!
});
```

✅ **Correct**: Queue for UI thread
```cpp
app.queue_ui_task([&element]() {
    element->measure(100, 100);  // Safe
});
```

### 5. Backend Thread Safety

Different backends may have different threading requirements:

**conio_backend** (DOS/Console):
- Single-threaded only
- All rendering on main thread

**Custom backends**:
- Check backend documentation
- Some may support multi-threaded rendering
- Coordinate with onyxui threading model

## Performance Considerations

### Signal Emission Overhead

With `ONYXUI_THREAD_SAFE=ON`:
- **Mutex overhead**: ~20-50ns per lock/unlock
- **Atomic ID generation**: ~5ns per connection
- **Slot copying**: O(n) where n = number of connections

**Optimization**: Batch emissions
```cpp
// Bad: High mutex overhead
for (int i = 0; i < 1000; i++) {
    signal.emit(i);  // 1000 lock/copy cycles
}

// Good: Use batching
std::vector<int> values;
for (int i = 0; i < 1000; i++) {
    values.push_back(i);
}
signal_batch.emit(values);  // 1 lock/copy cycle
```

### Thread-Local Signals

For thread-specific events, consider thread-local signals (future feature):
```cpp
// Each thread has its own signal instance
thread_local signal<> thread_event;
```

### Lock Contention

Profile mutex contention in multi-threaded scenarios:
- Use `perf` or `VTune` to identify hotspots
- Consider signal redesign if >5% time in locks
- Use thread-local signals where applicable

## Debugging Thread Safety Issues

### ThreadSanitizer (TSAN)

Build with TSAN to detect race conditions:
```bash
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build
./build/bin/ui_unittest
```

### Common Issues

**Data race in signal emission**:
```
WARNING: ThreadSanitizer: data race
  Write at signal.hh:XXX
  Previous read at signal.hh:YYY
```
- Solution: Enable ONYXUI_THREAD_SAFE

**Deadlock in callback**:
```
(program hangs)
```
- Solution: Don't call signal.disconnect() from within callback on same signal
- Workaround: Use scoped_connection in outer scope

**Use-after-free with scoped_connection**:
```
WARNING: ThreadSanitizer: heap-use-after-free
```
- Solution: Ensure signal outlives scoped_connection
- Pattern: Declare signal before scoped_connection in class

## Best Practices Summary

1. ✅ **Enable ONYXUI_THREAD_SAFE for multi-threaded applications**
2. ✅ **Signal operations can occur from any thread**
3. ✅ **Use message queue for cross-thread UI updates**
4. ✅ **Keep UI operations on main thread**
5. ✅ **Profile mutex contention for performance-critical code**
6. ✅ **Use TSAN for testing multi-threaded code**
7. ❌ **Don't modify UI tree from background threads**
8. ❌ **Don't call layout operations from background threads**
9. ❌ **Don't assume immediate emission visibility across threads**
10. ❌ **Don't create circular dependencies between signals**

## Examples

### Example 1: Background Data Processing with UI Update

```cpp
class DataProcessor {
    signal<std::string> m_status_changed;
    std::atomic<bool> m_running{false};

public:
    void start_processing() {
        m_running = true;
        std::thread worker([this]() {
            while (m_running) {
                auto result = process_data();
                // Safe: emit from background thread
                m_status_changed.emit("Processed: " + result);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        worker.detach();
    }

    void stop_processing() {
        m_running = false;
    }

    signal<std::string>& status_changed() { return m_status_changed; }
};

// In UI code (main thread)
DataProcessor processor;
processor.status_changed().connect([&status_label](const std::string& msg) {
    // This callback runs on worker thread!
    // Queue for UI thread instead
    app.queue_ui_task([&status_label, msg]() {
        status_label->set_text(msg);  // Safe on UI thread
    });
});
processor.start_processing();
```

### Example 2: Thread-Safe Event Aggregation

```cpp
class EventAggregator {
    signal<std::vector<std::string>> m_batch_ready;
    std::vector<std::string> m_batch;
    std::mutex m_batch_mutex;

public:
    void add_event(const std::string& event) {
        bool should_emit = false;
        {
            std::lock_guard lock(m_batch_mutex);
            m_batch.push_back(event);
            should_emit = (m_batch.size() >= 100);
        }

        if (should_emit) {
            std::vector<std::string> ready_batch;
            {
                std::lock_guard lock(m_batch_mutex);
                ready_batch = std::move(m_batch);
                m_batch.clear();
            }
            m_batch_ready.emit(ready_batch);  // Safe: signal is thread-safe
        }
    }
};
```

### Example 3: Proper Scoped Connection Lifetime

```cpp
class Widget {
    signal<> m_clicked;           // Signal declared first
    scoped_connection m_conn;     // Connection declared after

public:
    Widget() {
        // Safe: m_conn will be destroyed before m_clicked
        m_conn = scoped_connection(m_clicked, []() {
            std::cout << "Clicked\n";
        });
    }

    // Destructor: m_conn destroyed first, then m_clicked
    // This is safe because m_conn still has valid reference
};

// UNSAFE pattern:
class BadWidget {
    scoped_connection m_conn;     // Connection declared first
    signal<> m_clicked;           // Signal declared after

    // Destructor: m_clicked destroyed first, then m_conn
    // CRASH! m_conn tries to disconnect from destroyed signal
};
```

## Future Enhancements

Planned thread safety improvements:

1. **Thread-local signals**: Zero-overhead signals for thread-specific events
2. **Lock-free signal implementation**: For performance-critical paths
3. **Thread-safe layout manager**: Optional thread-safe layout calculations
4. **Read-only UI inspection**: Thread-safe queries for UI state

## Resources

- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition)
- [ThreadSanitizer Documentation](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- [std::shared_mutex Reference](https://en.cppreference.com/w/cpp/thread/shared_mutex)
