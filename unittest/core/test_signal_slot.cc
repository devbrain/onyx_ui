/**
 * @file test_signal_slot.cc
 * @brief Comprehensive tests for signal/slot system
 * @author igor
 * @date 16/10/2025
 */

#include <cstddef>
#include <doctest/doctest.h>
#include <../../include/onyxui/core/signal.hh>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace onyxui;

// ============================================================================
// Basic Signal Tests
// ============================================================================

TEST_CASE("Signal - Basic connection and emission") {
    signal<> simple_signal;
    int call_count = 0;

    SUBCASE("Single connection") {
        [[maybe_unused]] auto id = simple_signal.connect([&call_count]() {
            call_count++;
        });

        CHECK(simple_signal.has_connections());
        CHECK(simple_signal.connection_count() == 1);

        simple_signal.emit();
        CHECK(call_count == 1);

        simple_signal.emit();
        CHECK(call_count == 2);
    }

    SUBCASE("Multiple connections") {
        int call_count_a = 0;
        int call_count_b = 0;

        simple_signal.connect([&call_count_a]() { call_count_a++; });
        simple_signal.connect([&call_count_b]() { call_count_b++; });

        CHECK(simple_signal.connection_count() == 2);

        simple_signal.emit();
        CHECK(call_count_a == 1);
        CHECK(call_count_b == 1);
    }

    SUBCASE("Operator() emission") {
        simple_signal.connect([&call_count]() { call_count++; });

        simple_signal();  // Use operator() instead of emit()
        CHECK(call_count == 1);
    }

    SUBCASE("Empty signal emission") {
        // Should not crash when no connections
        CHECK_NOTHROW(simple_signal.emit());
        CHECK_NOTHROW(simple_signal());
    }
}

TEST_CASE("Signal - With arguments") {
    signal<int, std::string> data_signal;

    SUBCASE("Single argument capture") {
        int received_int = 0;
        std::string received_str;

        data_signal.connect([&](int value, const std::string& msg) {
            received_int = value;
            received_str = msg;
        });

        data_signal.emit(42, "hello");
        CHECK(received_int == 42);
        CHECK(received_str == "hello");

        data_signal(100, "world");
        CHECK(received_int == 100);
        CHECK(received_str == "world");
    }

    SUBCASE("Multiple slots with arguments") {
        std::vector<int> received_ints;

        data_signal.connect([&](int value, const std::string&) {
            received_ints.push_back(value);
        });

        data_signal.connect([&](int value, const std::string&) {
            received_ints.push_back(value * 2);
        });

        data_signal.emit(5, "test");
        CHECK(received_ints.size() == 2);
        // Check that both values are present (order is undefined)
        bool const has_5 = (received_ints[0] == 5 || received_ints[1] == 5);
        bool const has_10 = (received_ints[0] == 10 || received_ints[1] == 10);
        CHECK(has_5);
        CHECK(has_10);
    }
}

TEST_CASE("Signal - Complex argument types") {
    struct ComplexData {
        int id = 0;
        std::string name;
        std::vector<int> values;
    };

    signal<const ComplexData&> complex_signal;

    ComplexData received;
    complex_signal.connect([&](const ComplexData& data) {
        received = data;
    });

    ComplexData const sent{42, "test", {1, 2, 3}};
    complex_signal.emit(sent);

    CHECK(received.id == 42);
    CHECK(received.name == "test");
    CHECK(received.values.size() == 3);
    CHECK(received.values[0] == 1);
}

// ============================================================================
// Disconnect Tests
// ============================================================================

TEST_CASE("Signal - Disconnect by ID") {
    signal<int> int_signal;
    int call_count = 0;

    auto id = int_signal.connect([&](int value) {
        call_count += value;
    });

    int_signal.emit(5);
    CHECK(call_count == 5);

    int_signal.disconnect(id);
    CHECK(int_signal.connection_count() == 0);
    CHECK_FALSE(int_signal.has_connections());

    int_signal.emit(10);
    CHECK(call_count == 5);  // Should not have changed
}

TEST_CASE("Signal - Disconnect all") {
    signal<> simple_signal;
    int count_a = 0;
    int count_b = 0;
    int count_c = 0;

    simple_signal.connect([&]() { count_a++; });
    simple_signal.connect([&]() { count_b++; });
    simple_signal.connect([&]() { count_c++; });

    CHECK(simple_signal.connection_count() == 3);

    simple_signal.disconnect_all();
    CHECK(simple_signal.connection_count() == 0);

    simple_signal.emit();
    CHECK(count_a == 0);
    CHECK(count_b == 0);
    CHECK(count_c == 0);
}

TEST_CASE("Signal - Disconnect non-existent ID") {
    signal<> simple_signal;

    // Disconnecting non-existent ID should not crash
    CHECK_NOTHROW(simple_signal.disconnect(999));

    auto id = simple_signal.connect([]() {});
    simple_signal.disconnect(id);

    // Disconnecting same ID twice should not crash
    CHECK_NOTHROW(simple_signal.disconnect(id));
}

TEST_CASE("Signal - Self-disconnect during emission") {
    signal<> simple_signal;
    int call_count = 0;
    signal<>::connection_id my_id = 0;

    my_id = simple_signal.connect([&]() {
        call_count++;
        // Disconnect self during callback
        simple_signal.disconnect(my_id);
    });

    simple_signal.emit();
    CHECK(call_count == 1);

    // Should not be called again
    simple_signal.emit();
    CHECK(call_count == 1);
}

// ============================================================================
// Scoped Connection Tests
// ============================================================================

TEST_CASE("Scoped connection - Basic RAII") {
    signal<int> int_signal;
    int call_count = 0;

    {
        scoped_connection const conn(int_signal, [&](int value) {
            call_count += value;
        });

        CHECK(conn.is_connected());

        int_signal.emit(5);
        CHECK(call_count == 5);
    }  // conn destroyed here, should auto-disconnect

    int_signal.emit(10);
    CHECK(call_count == 5);  // Should not have changed
}

TEST_CASE("Scoped connection - Manual disconnect") {
    signal<> simple_signal;
    int call_count = 0;

    scoped_connection conn(simple_signal, [&]() {
        call_count++;
    });

    simple_signal.emit();
    CHECK(call_count == 1);

    conn.disconnect();
    CHECK_FALSE(conn.is_connected());

    simple_signal.emit();
    CHECK(call_count == 1);  // Should not have changed

    // Calling disconnect again should be safe
    CHECK_NOTHROW(conn.disconnect());
}

TEST_CASE("Scoped connection - Move semantics") {
    signal<> simple_signal;
    int call_count = 0;

    scoped_connection conn1(simple_signal, [&]() {
        call_count++;
    });

    // Move to another scoped_connection
    scoped_connection conn2 = std::move(conn1);

    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    CHECK_FALSE(conn1.is_connected());  // Intentionally testing moved-from state
    CHECK(conn2.is_connected());

    simple_signal.emit();
    CHECK(call_count == 1);

    // conn2 should still control the connection
    conn2.disconnect();
    simple_signal.emit();
    CHECK(call_count == 1);
}

TEST_CASE("Scoped connection - Move assignment") {
    signal<> signal1;
    signal<> signal2;
    int count1 = 0;
    int count2 = 0;

    scoped_connection conn1(signal1, [&]() { count1++; });
    scoped_connection conn2(signal2, [&]() { count2++; });

    signal1.emit();
    signal2.emit();
    CHECK(count1 == 1);
    CHECK(count2 == 1);

    // Move assign - should disconnect from signal2 and take over signal1 connection
    conn2 = std::move(conn1);

    signal1.emit();
    signal2.emit();
    CHECK(count1 == 2);  // conn2 now controls signal1 connection
    CHECK(count2 == 1);  // Old signal2 connection was disconnected
}

TEST_CASE("Scoped connection - Default constructor") {
    scoped_connection conn;  // Empty connection

    CHECK_FALSE(conn.is_connected());
    CHECK_NOTHROW(conn.disconnect());  // Should be safe
}

TEST_CASE("Scoped connection - Multiple in same scope") {
    signal<> sig;
    int count_a = 0;
    int count_b = 0;
    int count_c = 0;

    {
        scoped_connection const conn_a(sig, [&]() { count_a++; });
        scoped_connection const conn_b(sig, [&]() { count_b++; });
        scoped_connection const conn_c(sig, [&]() { count_c++; });

        sig.emit();
        CHECK(count_a == 1);
        CHECK(count_b == 1);
        CHECK(count_c == 1);
    }  // All auto-disconnect

    sig.emit();
    CHECK(count_a == 1);
    CHECK(count_b == 1);
    CHECK(count_c == 1);
}

// ============================================================================
// Advanced Use Cases
// ============================================================================

TEST_CASE("Signal - Lambda with captures") {
    signal<int> multiplier_signal;

    int factor = 5;
    int result = 0;

    multiplier_signal.connect([&, factor](int value) {
        result = value * factor;
    });

    multiplier_signal.emit(10);
    CHECK(result == 50);

    // Change captured variable
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    factor = 3;  // Intentionally not used - testing that capture-by-value made a copy
    multiplier_signal.emit(10);
    CHECK(result == 50);  // Still 50 because factor was captured by value
}

TEST_CASE("Signal - Connecting during emission") {
    signal<> simple_signal;
    int first_count = 0;
    int second_count = 0;

    simple_signal.connect([&]() {
        first_count++;

        // Connect another slot during emission
        simple_signal.connect([&]() {
            second_count++;
        });
    });

    simple_signal.emit();
    CHECK(first_count == 1);
    CHECK(second_count == 0);  // New connection not called yet

    simple_signal.emit();
    CHECK(first_count == 2);
    CHECK(second_count == 1);  // Now called
}

TEST_CASE("Signal - State changes in slots") {
    signal<int> value_changed;

    std::vector<int> history;
    int sum = 0;

    value_changed.connect([&](int value) {
        history.push_back(value);
    });

    value_changed.connect([&](int value) {
        sum += value;
    });

    value_changed.emit(10);
    value_changed.emit(20);
    value_changed.emit(30);

    CHECK(history.size() == 3);
    CHECK(history[0] == 10);
    CHECK(history[1] == 20);
    CHECK(history[2] == 30);
    CHECK(sum == 60);
}

TEST_CASE("Signal - Return values are ignored") {
    // Slots can have return values but they're ignored
    signal<int> int_signal;

    int_signal.connect([](int value) -> int {
        return value * 2;  // Return value is ignored
    });

    // Should compile and run without issues
    CHECK_NOTHROW(int_signal.emit(42));
}

// ============================================================================
// Real-World Scenarios
// ============================================================================

TEST_CASE("Real-world - Button click simulation") {
    struct Button {
        signal<> clicked;

        void click() {
            clicked.emit();
        }
    };

    Button button;
    int click_count = 0;

    button.clicked.connect([&]() {
        click_count++;
    });

    button.click();
    button.click();
    button.click();

    CHECK(click_count == 3);
}

TEST_CASE("Real-world - Observer pattern") {
    struct DataModel {
        signal<int> value_changed;

        void set_value(int new_value) {
            m_value = new_value;
            value_changed.emit(new_value);
        }

        int get_value() const { return m_value; }

    private:
        int m_value = 0;
    };

    DataModel model;
    std::vector<int> observed_values;

    model.value_changed.connect([&](int value) {
        observed_values.push_back(value);
    });

    model.set_value(10);
    model.set_value(20);
    model.set_value(30);

    CHECK(observed_values.size() == 3);
    CHECK(observed_values[0] == 10);
    CHECK(observed_values[1] == 20);
    CHECK(observed_values[2] == 30);
}

TEST_CASE("Real-world - Multiple listeners") {
    struct EventSource {
        signal<const std::string&> event_occurred;
    };

    EventSource source;
    std::vector<std::string> log;
    int alert_count = 0;

    // Logger
    source.event_occurred.connect([&](const std::string& event) {
        log.push_back("LOG: " + event);
    });

    // Alert system
    source.event_occurred.connect([&](const std::string& event) {
        if (event == "ERROR") {
            alert_count++;
        }
    });

    // Analytics
    std::vector<std::string> analytics;
    source.event_occurred.connect([&](const std::string& event) {
        analytics.push_back(event);
    });

    source.event_occurred.emit("START");
    source.event_occurred.emit("ERROR");
    source.event_occurred.emit("ERROR");
    source.event_occurred.emit("STOP");

    CHECK(log.size() == 4);
    CHECK(alert_count == 2);
    CHECK(analytics.size() == 4);
}

TEST_CASE("Real-world - Class member connection lifecycle") {
    struct Controller {
        signal<int> value_updated;
    };

    struct View {
        scoped_connection update_connection;
        int last_value = 0;

        void bind_to_controller(Controller& controller) {
            update_connection = scoped_connection(
                controller.value_updated,
                [this](int value) { on_value_changed(value); }
            );
        }

        void on_value_changed(int value) {
            last_value = value;
        }
    };

    Controller controller;

    {
        View view;
        view.bind_to_controller(controller);

        controller.value_updated.emit(42);
        CHECK(view.last_value == 42);
    }  // View destroyed, connection auto-disconnected

    // Emitting should not crash even though view is destroyed
    CHECK_NOTHROW(controller.value_updated.emit(100));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("Edge case - Many connections") {
    signal<int> sig;
    std::vector<int> results(100, 0);

    // Connect 100 slots
    for (size_t i = 0; i < 100; ++i) {
        sig.connect([&results, i](int value) {
            results[i] = value + static_cast<int>(i);
        });
    }

    CHECK(sig.connection_count() == 100);

    sig.emit(10);

    for (size_t i = 0; i < 100; ++i) {
        CHECK(results[i] == 10 + static_cast<int>(i));
    }
}

TEST_CASE("Edge case - Disconnect all during emission") {
    signal<> sig;
    int count = 0;

    sig.connect([&]() {
        count++;
        sig.disconnect_all();  // Clear all during emission
    });

    sig.connect([&]() {
        count++;  // This might or might not be called depending on order
    });

    CHECK_NOTHROW(sig.emit());
    // Count should be at least 1
    CHECK(count >= 1);
}

TEST_CASE("Edge case - Connection ID uniqueness") {
    signal<> sig;

    auto id1 = sig.connect([]() {});
    auto id2 = sig.connect([]() {});
    auto id3 = sig.connect([]() {});

    // IDs should be unique
    CHECK(id1 != id2);
    CHECK(id2 != id3);
    CHECK(id1 != id3);
}

// ============================================================================
// Lifetime Safety Tests (Phase 1.1 Refactoring)
// ============================================================================

TEST_CASE("scoped_connection - Signal destroyed before connection") {
    std::optional<scoped_connection> conn;

    {
        signal<int> sig;
        conn.emplace(sig, [](int) {});
        // sig destroyed here
    }

    // conn destroyed here - should NOT crash
    CHECK_NOTHROW(conn.reset());
}

TEST_CASE("scoped_connection - Connection destroyed before signal") {
    signal<int> sig;
    int call_count = 0;

    {
        scoped_connection conn(sig, [&](int) {
            ++call_count;
        });
        // conn destroyed here - should disconnect cleanly
    }

    sig.emit(42);  // Should not call disconnected slot
    CHECK(call_count == 0);
    CHECK(sig.connection_count() == 0);
}

TEST_CASE("scoped_connection - Multiple connections with signal destruction") {
    std::optional<scoped_connection> conn1;
    std::optional<scoped_connection> conn2;
    std::optional<scoped_connection> conn3;

    {
        signal<> sig;
        conn1.emplace(sig, []() {});
        conn2.emplace(sig, []() {});
        conn3.emplace(sig, []() {});
        CHECK(sig.connection_count() == 3);
        // sig destroyed here - all connections should handle it gracefully
    }

    // Destroying connections after signal destruction should be safe
    CHECK_NOTHROW(conn1.reset());
    CHECK_NOTHROW(conn2.reset());
    CHECK_NOTHROW(conn3.reset());
}

TEST_CASE("scoped_connection - Manual disconnect after signal destruction") {
    std::optional<scoped_connection> conn;

    {
        signal<int> sig;
        conn.emplace(sig, [](int) {});
        // sig destroyed here
    }

    // Manually calling disconnect after signal destruction should be safe
    CHECK_NOTHROW(conn->disconnect());
    CHECK_NOTHROW(conn->disconnect());  // Multiple calls should also be safe
}

TEST_CASE("scoped_connection - Move after signal destruction") {
    std::optional<scoped_connection> conn1;

    {
        signal<> sig;
        conn1.emplace(sig, []() {});
        // sig destroyed here
    }

    // Moving a connection after signal destruction should be safe
    CHECK_NOTHROW({
        scoped_connection conn2 = std::move(*conn1);
        // conn2 destroyed here
    });
}

TEST_CASE("scoped_connection - Alive flag behavior") {
    signal<int> sig;

    // Get alive flag while signal is alive
    auto alive_flag = sig.get_alive_flag();
    CHECK(alive_flag != nullptr);
    CHECK(*alive_flag == true);

    std::shared_ptr<bool> temp_flag;
    {
        signal<int> temp_sig;
        temp_flag = temp_sig.get_alive_flag();
        CHECK(*temp_flag == true);
        // temp_sig destroyed here
    }
    // Now check after temp_sig has been destroyed
    CHECK(*temp_flag == false);  // Flag should be set to false

    // Original signal still alive
    CHECK(*alive_flag == true);
}
