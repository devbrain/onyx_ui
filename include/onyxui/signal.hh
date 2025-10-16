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
     * ⚠️ **CRITICAL: This implementation is NOT thread-safe!**
     *
     * All operations (connect, disconnect, emit) MUST be performed from the same
     * thread, typically the UI thread. Using signals from multiple threads without
     * external synchronization will result in undefined behavior due to data races.
     *
     * ### Specific Risks:
     *
     * 1. **connect()**: Non-atomic increment of `m_next_id` (line 120) can cause
     *    connection ID collisions if called from multiple threads
     *
     * 2. **disconnect()**: Concurrent modifications to `m_slots` map during iteration
     *    can cause iterator invalidation and crashes
     *
     * 3. **emit()**: While the implementation copies `m_slots` before iteration to
     *    handle disconnect-during-emit, this does NOT protect against concurrent
     *    connect()/disconnect() calls from other threads
     *
     * 4. **Shared state**: `m_slots` and `m_next_id` have no mutex protection
     *
     * ### Safe Usage Patterns:
     *
     * - ✅ All signal operations on UI/main thread
     * - ✅ Worker threads post to main thread to emit signals
     * - ✅ Use external mutex if multi-threaded access is required
     * - ❌ Direct emit() from worker threads
     * - ❌ connect()/disconnect() from background threads
     *
     * @warning If you need thread-safe signals, add a `std::mutex` and lock it in
     *          connect(), disconnect(), and emit(). Note this will impact performance.
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
            signal(signal&&) noexcept = default;
            signal& operator=(signal&&) noexcept = default;

        private:
            std::unordered_map <connection_id, slot_type> m_slots;
            connection_id m_next_id = 0;
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
             * @example
             * @code
             * signal<int> value_changed;
             * auto id = value_changed.connect([](int new_value) {
             *     std::cout << "Value: " << new_value << std::endl;
             * });
             * @endcode
             */
            connection_id connect(slot_type slot) {
                connection_id id = m_next_id++;
                m_slots[id] = std::move(slot);
                return id;
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
             * @example
             * @code
             * auto id = my_signal.connect(handler);
             * // ... later ...
             * my_signal.disconnect(id);  // Handler no longer called
             * @endcode
             */
            void disconnect(connection_id id) {
                m_slots.erase(id);
            }

            /**
             * @brief Disconnect all slots
             *
             * @details
             * Removes all connected slots. After this call, the signal will have
             * no effect when emitted until new slots are connected.
             */
            void disconnect_all() {
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
             * @example
             * @code
             * if (expensive_computation_complete.has_connections()) {
             *     auto result = perform_expensive_computation();
             *     expensive_computation_complete.emit(result);
             * }
             * @endcode
             */
            [[nodiscard]] bool has_connections() const noexcept {
                return !m_slots.empty();
            }

            /**
             * @brief Get the number of connected slots
             *
             * @return The number of active connections
             */
            [[nodiscard]] size_t connection_count() const noexcept {
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
             */
            void disconnect() {
                if (m_disconnect) {
                    m_disconnect();
                    m_disconnect = nullptr;
                }
            }

            /**
             * @brief Check if this holds an active connection
             *
             * @return true if connected, false if empty or disconnected
             */
            [[nodiscard]] bool is_connected() const noexcept {
                return m_disconnect != nullptr;
            }

        private:
            std::function <void()> m_disconnect;
    };
}
