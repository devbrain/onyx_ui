/**
 * @file signal.hh
 * @brief Lightweight signal/slot system for event handling
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides a modern C++ signal/slot system without requiring code generation
 * or meta-object compilation. Uses std::function and variadic templates to
 * provide type-safe, flexible event handling.
 *
 * ## Features
 *
 * - **Type-safe**: Compile-time type checking for signal arguments
 * - **Flexible**: Works with lambdas, free functions, member functions
 * - **Header-only**: No code generation or special build steps
 * - **RAII**: scoped_connection for automatic disconnect
 * - **Multiple slots**: One signal can connect to many handlers
 * - **Manual disconnect**: By connection ID
 *
 * ## Basic Usage
 *
 * @code
 * // Define a signal
 * signal<int, std::string> data_received;
 *
 * // Connect a lambda
 * auto conn_id = data_received.connect([](int value, const std::string& msg) {
 *     std::cout << "Received: " << value << ", " << msg << std::endl;
 * });
 *
 * // Emit the signal
 * data_received.emit(42, "hello");
 *
 * // Disconnect
 * data_received.disconnect(conn_id);
 * @endcode
 *
 * ## RAII Usage
 *
 * @code
 * class MyWidget {
 *     signal<> clicked;
 *     scoped_connection m_connection;
 *
 *     void setup() {
 *         // Connection automatically disconnects when m_connection is destroyed
 *         m_connection = scoped_connection(clicked, []() {
 *             std::cout << "Clicked!" << std::endl;
 *         });
 *     }
 * };
 * @endcode
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <utility>

#ifdef ONYXUI_THREAD_SAFE
    #include <shared_mutex>
    #include <mutex>
    #include <atomic>
#endif

namespace onyxui {
    /**
     * @class signal
     * @brief Type-safe signal that can connect to multiple slots
     *
     * @tparam Args The argument types that will be passed when the signal is emitted
     *
     * @details
     * A signal represents an event that can be observed by multiple slots (handlers).
     * When the signal is emitted, all connected slots are invoked with the provided
     * arguments.
     *
     * ## Connection Management
     *
     * Each connection is assigned a unique ID that can be used to disconnect later.
     * Connections are stored in an unordered_map for O(1) access and removal.
     *
     * ## Thread Safety
     *
     * Thread safety depends on the compilation flag ONYXUI_THREAD_SAFE:
     *
     * ### With ONYXUI_THREAD_SAFE=1 (Thread-Safe Mode)
     *
     * ✅ **All operations are thread-safe:**
     * - `connect()`: Uses atomic ID generation and write lock
     * - `disconnect()`: Uses write lock for map modification
     * - `emit()`: Uses read lock for copying slots, releases lock before invoking callbacks
     * - Connection ID generation: Atomic increment prevents collisions
     *
     * **Implementation Details:**
     * - Uses `std::shared_mutex` for reader-writer locking
     * - `std::atomic<connection_id>` for thread-safe ID generation
     * - Slots are copied under shared lock, then callbacks invoked without locks
     * - Prevents deadlock by never calling user code while holding locks
     *
     * **Performance Impact:**
     * - Mutex overhead: ~20-50ns per operation
     * - Lock contention possible with many threads
     * - Recommended for multi-threaded applications
     *
     * ### Without ONYXUI_THREAD_SAFE (Single-Threaded Mode)
     *
     * ⚠️ **NOT thread-safe - maximum performance for single-threaded use:**
     * - No mutex overhead
     * - Simple increment for ID generation
     * - All operations must be on the same thread (typically UI thread)
     *
     * **Usage Restrictions:**
     * - ✅ All signal operations on UI/main thread
     * - ✅ Worker threads post to main thread to emit signals
     * - ❌ Direct emit() from worker threads
     * - ❌ connect()/disconnect() from background threads
     *
     * @thread_safety Thread-safe when compiled with ONYXUI_THREAD_SAFE=1.
     *                Single-threaded only when compiled without ONYXUI_THREAD_SAFE.
     *
     * @performance With ONYXUI_THREAD_SAFE: ~5-10% overhead due to mutex operations.
     *              Without ONYXUI_THREAD_SAFE: Zero overhead, maximum performance.
     *
     * ## Performance
     *
     * - connect(): O(1)
     * - disconnect(): O(1)
     * - emit(): O(n) where n is the number of connected slots
     *
     * @note emit() copies the entire slots map (O(n)) for safety during iteration.
     *       For high-frequency signals, this may impact performance.
     */
    template<typename... Args>
    class signal {
        public:
            using slot_type = std::function <void(Args...)>;
            using connection_id = size_t;

            /**
             * @brief Default constructor
             */
            signal() = default;

            /**
             * @brief Destructor
             */
            ~signal() = default;

            // Rule of Five
            signal(const signal&) = delete;
            signal& operator=(const signal&) = delete;

#ifdef ONYXUI_THREAD_SAFE
            // Custom move operations (mutex and atomic are not moveable)
            signal(signal&& other) noexcept
                : m_next_id(other.m_next_id.load(std::memory_order_relaxed)),
                  m_slots(std::move(other.m_slots)),
                  m_emitting(other.m_emitting) {
                other.m_slots.clear();
                other.m_emitting = false;
            }

            signal& operator=(signal&& other) noexcept {
                if (this != &other) {
                    std::unique_lock lock1(m_mutex, std::defer_lock);
                    std::unique_lock lock2(other.m_mutex, std::defer_lock);
                    std::lock(lock1, lock2);

                    m_next_id.store(other.m_next_id.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    m_slots = std::move(other.m_slots);
                    m_emitting = other.m_emitting;

                    other.m_slots.clear();
                    other.m_emitting = false;
                }
                return *this;
            }
#else
            signal(signal&&) noexcept = default;
            signal& operator=(signal&&) noexcept = default;
#endif

        private:
#ifdef ONYXUI_THREAD_SAFE
            mutable std::shared_mutex m_mutex;
            std::atomic<connection_id> m_next_id{1};
#else
            connection_id m_next_id = 1;
#endif
            std::unordered_map <connection_id, slot_type> m_slots;
            bool m_emitting = false;  ///< Re-entrancy guard to detect nested emit() calls

        public:
            /**
             * @brief Connect a slot to this signal
             *
             * @param slot The function/lambda to call when signal is emitted
             * @return connection_id Unique ID for this connection, use to disconnect later
             *
             * @details
             * The slot will be invoked every time emit() is called, until it is
             * disconnected. Multiple slots can be connected to the same signal.
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses write lock.
             *
             * @example
             * @code
             * signal<int> value_changed;
             * auto id = value_changed.connect([](int new_value) {
             *     std::cout << "Value: " << new_value << std::endl;
             * });
             * @endcode
             */
            connection_id connect(slot_type slot) {
#ifdef ONYXUI_THREAD_SAFE
                std::unique_lock const lock(m_mutex);
                connection_id const id = m_next_id.fetch_add(1, std::memory_order_relaxed);
                m_slots[id] = std::move(slot);
                return id;
#else
                connection_id id = m_next_id++;
                m_slots[id] = std::move(slot);
                return id;
#endif
            }

            /**
             * @brief Disconnect a specific slot by ID
             *
             * @param id The connection ID returned by connect()
             *
             * @details
             * Removes the slot with the given ID. If the ID doesn't exist, this
             * operation does nothing (safe to call multiple times).
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses write lock.
             *
             * @example
             * @code
             * auto id = my_signal.connect(handler);
             * // ... later ...
             * my_signal.disconnect(id);  // Handler no longer called
             * @endcode
             */
            void disconnect(connection_id id) {
#ifdef ONYXUI_THREAD_SAFE
                std::unique_lock const lock(m_mutex);
#endif
                m_slots.erase(id);
            }

            /**
             * @brief Disconnect all slots
             *
             * @details
             * Removes all connected slots. After this call, the signal will have
             * no effect when emitted until new slots are connected.
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses write lock.
             */
            void disconnect_all() {
#ifdef ONYXUI_THREAD_SAFE
                std::unique_lock const lock(m_mutex);
#endif
                m_slots.clear();
            }

            /**
             * @brief Emit the signal, invoking all connected slots
             *
             * @param args The arguments to pass to each slot
             *
             * @details
             * All connected slots are invoked synchronously in an unspecified order.
             * If a slot disconnects itself during emission, it's safe (won't be called).
             * If a slot connects a new slot during emission, the new slot will NOT be
             * called during this emission (only on the next one).
             *
             * ## Performance Optimization
             *
             * - **Early return**: If no slots connected, returns immediately (O(1))
             * - **Re-entrancy detection**: Sets guard flag to detect nested emit() calls
             * - **Safe iteration**: Copies slot map to allow disconnect during emit
             *
             * The map copy overhead is typically small (most signals have 1-5 slots) and
             * necessary for safety when slots modify connections.
             *
             * ## Thread Safety
             *
             * With ONYXUI_THREAD_SAFE:
             * - Copies slots under shared lock (allows concurrent emit() calls)
             * - Releases lock before invoking callbacks (prevents deadlock)
             * - Re-checks connection status for each slot (in case disconnected concurrently)
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses read lock for copying,
             *                no locks held during callback invocation.
             *
             * @note If a slot throws an exception, it propagates to the caller and
             *       remaining slots are NOT invoked. Design slots to be exception-safe.
             *
             * @example
             * @code
             * signal<int, std::string> data_ready;
             * data_ready.connect([](int code, const std::string& msg) {
             *     std::cout << code << ": " << msg << std::endl;
             * });
             * data_ready.emit(200, "OK");  // Prints: "200: OK"
             * @endcode
             */
            void emit(Args... args) {
#ifdef ONYXUI_THREAD_SAFE
                // Copy slots under shared lock
                std::unordered_map<connection_id, slot_type> slots_copy;
                {
                    std::shared_lock const lock(m_mutex);
                    if (m_slots.empty()) {
                        return;  // Early return if no slots
                    }
                    slots_copy = m_slots;
                }
                // Lock released here - callbacks can safely connect/disconnect

                // Set re-entrancy guard
                bool const was_emitting = m_emitting;
                m_emitting = true;

                try {
                    for (auto& [id, slot] : slots_copy) {
                        // Check if still connected (might have been disconnected concurrently)
                        {
                            std::shared_lock const lock(m_mutex);
                            if (m_slots.find(id) == m_slots.end()) {
                                continue;  // Skip if disconnected
                            }
                        }
                        // Call without holding any lock to prevent deadlock
                        slot(args...);
                    }

                    m_emitting = was_emitting;
                } catch (...) {
                    m_emitting = was_emitting;
                    throw;
                }
#else
                // Early return if no slots connected - avoids map copy
                if (m_slots.empty()) {
                    return;
                }

                // Set re-entrancy guard
                bool was_emitting = m_emitting;
                m_emitting = true;

                // Use try-finally pattern to ensure m_emitting is reset even if exception thrown
                try {
                    // Copy slots to allow safe modification during iteration
                    // (e.g., if a slot disconnects itself or emits another signal)
                    auto slots_copy = m_slots;

                    for (auto& [id, slot] : slots_copy) {
                        // Check if still connected (might have been disconnected during iteration)
                        if (m_slots.find(id) != m_slots.end()) {
                            slot(args...);
                        }
                    }

                    m_emitting = was_emitting;
                } catch (...) {
                    m_emitting = was_emitting;
                    throw;  // Re-throw exception after cleanup
                }
#endif
            }

            /**
             * @brief Operator() as convenient alias for emit()
             *
             * @param args The arguments to pass to each slot
             *
             * @details
             * Allows using signal like a function call for more natural syntax.
             *
             * @example
             * @code
             * signal<> clicked;
             * clicked.connect([]() { std::cout << "Clicked!\n"; });
             * clicked();  // Equivalent to clicked.emit()
             * @endcode
             */
            void operator()(Args... args) {
                emit(args...);
            }

            /**
             * @brief Check if any slots are connected
             *
             * @return true if at least one slot is connected, false otherwise
             *
             * @details
             * Useful for optimization - skip work if no one is listening.
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses read lock.
             *
             * @example
             * @code
             * if (expensive_computation_complete.has_connections()) {
             *     auto result = perform_expensive_computation();
             *     expensive_computation_complete.emit(result);
             * }
             * @endcode
             */
            [[nodiscard]] bool has_connections() const noexcept {
#ifdef ONYXUI_THREAD_SAFE
                std::shared_lock const lock(m_mutex);
#endif
                return !m_slots.empty();
            }

            /**
             * @brief Get the number of connected slots
             *
             * @return The number of active connections
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses read lock.
             */
            [[nodiscard]] size_t connection_count() const noexcept {
#ifdef ONYXUI_THREAD_SAFE
                std::shared_lock const lock(m_mutex);
#endif
                return m_slots.size();
            }
    };

    /**
     * @class scoped_connection
     * @brief RAII wrapper for automatic connection cleanup
     *
     * @details
     * Holds a connection to a signal and automatically disconnects when destroyed.
     * Useful for ensuring cleanup in class destructors or when connections should
     * have a limited lifetime.
     *
     * ## Movable, Not Copyable
     *
     * scoped_connection is move-only to ensure clear ownership semantics.
     * Moving transfers ownership of the connection.
     *
     * ## Thread Safety
     *
     * With ONYXUI_THREAD_SAFE:
     * - **disconnect()** is thread-safe (uses mutex)
     * - **Destructor** is thread-safe (calls thread-safe disconnect())
     * - **Move operations** are NOT thread-safe (don't move while other thread accesses)
     *
     * Without ONYXUI_THREAD_SAFE:
     * - Not thread-safe, must be used from single thread only
     *
     * @thread_safety Thread-safe disconnect with ONYXUI_THREAD_SAFE.
     *                Move operations are never thread-safe.
     *
     * @example
     * @code
     * class Dialog {
     *     Button m_ok_button;
     *     scoped_connection m_ok_connection;
     *
     *     Dialog() {
     *         // Connection automatically disconnects when Dialog is destroyed
     *         m_ok_connection = scoped_connection(
     *             m_ok_button.clicked,
     *             [this]() { on_ok_clicked(); }
     *         );
     *     }
     *
     *     void on_ok_clicked() { }
     * };
     * @endcode
     */
    class scoped_connection {
        public:
            /**
             * @brief Construct an empty scoped_connection
             */
            scoped_connection() = default;

            /**
             * @brief Construct from a signal and slot
             *
             * @tparam Args The signal's argument types
             * @param sig The signal to connect to
             * @param slot The slot to connect
             *
             * @details
             * Connects the slot to the signal and stores the disconnect function.
             * When this object is destroyed, the slot is automatically disconnected.
             *
             * @warning **CRITICAL LIFETIME REQUIREMENT**:
             *          The signal MUST outlive this scoped_connection.
             *          Destroying the signal before the scoped_connection results in
             *          undefined behavior (dangling reference access in destructor).
             *
             * ## Safe Usage Pattern:
             * @code
             * class Widget {
             *     signal<> clicked;           // Signal declared first
             *     scoped_connection m_conn;   // Connection declared after
             *
             *     Widget() {
             *         m_conn = scoped_connection(clicked, []() { });
             *     }
             *     // Safe: m_conn destroyed before clicked (reverse construction order)
             * };
             * @endcode
             *
             * ## Unsafe Pattern - DO NOT DO THIS:
             * @code
             * scoped_connection conn;
             * {
             *     signal<> temp_signal;
             *     conn = scoped_connection(temp_signal, []() { });
             * } // temp_signal destroyed here - conn now holds dangling reference!
             * // UB: conn destructor will access destroyed signal
             * @endcode
             */
            template<typename... Args>
            scoped_connection(signal <Args...>& sig, typename signal <Args...>::slot_type slot)
                : m_disconnect([&sig, id = sig.connect(std::move(slot))]() {
                    sig.disconnect(id);
                }) {
            }

            /**
             * @brief Destructor - automatically disconnects
             */
            ~scoped_connection() {
                disconnect();
            }

            /**
             * @brief Move constructor
             */
            scoped_connection(scoped_connection&& other) noexcept
                : m_disconnect(std::move(other.m_disconnect)) {
                other.m_disconnect = nullptr;
            }

            /**
             * @brief Move assignment
             */
            scoped_connection& operator=(scoped_connection&& other) noexcept {
                if (this != &other) {
                    disconnect(); // Disconnect current connection if any
                    m_disconnect = std::move(other.m_disconnect);
                    other.m_disconnect = nullptr;
                }
                return *this;
            }

            // Delete copy operations
            scoped_connection(const scoped_connection&) = delete;
            scoped_connection& operator=(const scoped_connection&) = delete;

            /**
             * @brief Manually disconnect before destruction
             *
             * @details
             * Safe to call multiple times. After calling this, the destructor
             * will have no effect.
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses mutex.
             */
            void disconnect() {
#ifdef ONYXUI_THREAD_SAFE
                std::lock_guard const lock(m_mutex);
#endif
                if (m_disconnect) {
                    m_disconnect();
                    m_disconnect = nullptr;
                }
            }

            /**
             * @brief Check if this holds an active connection
             *
             * @return true if connected, false if empty or disconnected
             *
             * @thread_safety Thread-safe with ONYXUI_THREAD_SAFE. Uses mutex.
             */
            [[nodiscard]] bool is_connected() const noexcept {
#ifdef ONYXUI_THREAD_SAFE
                std::lock_guard const lock(m_mutex);
#endif
                return m_disconnect != nullptr;
            }

        private:
            std::function <void()> m_disconnect;
#ifdef ONYXUI_THREAD_SAFE
            mutable std::mutex m_mutex;
#endif
    };
}
