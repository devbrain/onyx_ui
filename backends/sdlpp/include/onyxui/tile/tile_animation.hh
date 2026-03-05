/**
 * @file tile_animation.hh
 * @brief Animation system for tile-based UI
 *
 * Provides animation primitives for game UIs:
 * - Easing functions (linear, ease-in/out, bounce, elastic)
 * - Property animations (float, int, position)
 * - Frame animations (sprite cycling)
 * - Animation controller for managing active animations
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <chrono>
#include <string>
#include <numbers>

namespace onyxui::tile {

// ============================================================================
// Easing Functions
// ============================================================================

/**
 * @enum easing_type
 * @brief Standard easing function types
 */
enum class easing_type {
    linear,
    ease_in_quad,
    ease_out_quad,
    ease_in_out_quad,
    ease_in_cubic,
    ease_out_cubic,
    ease_in_out_cubic,
    ease_in_sine,
    ease_out_sine,
    ease_in_out_sine,
    ease_in_expo,
    ease_out_expo,
    ease_in_out_expo,
    ease_in_back,
    ease_out_back,
    ease_in_out_back,
    ease_in_bounce,
    ease_out_bounce,
    ease_in_out_bounce,
    ease_in_elastic,
    ease_out_elastic,
    ease_in_out_elastic
};

/**
 * @namespace easing
 * @brief Easing function implementations
 *
 * All functions take t in [0,1] and return value in [0,1]
 * (may overshoot for elastic/back/bounce)
 */
namespace easing {

/// Linear interpolation (no easing)
[[nodiscard]] inline constexpr float linear(float t) noexcept {
    return t;
}

/// Quadratic ease-in
[[nodiscard]] inline constexpr float ease_in_quad(float t) noexcept {
    return t * t;
}

/// Quadratic ease-out
[[nodiscard]] inline constexpr float ease_out_quad(float t) noexcept {
    return t * (2.0f - t);
}

/// Quadratic ease-in-out
[[nodiscard]] inline constexpr float ease_in_out_quad(float t) noexcept {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

/// Cubic ease-in
[[nodiscard]] inline constexpr float ease_in_cubic(float t) noexcept {
    return t * t * t;
}

/// Cubic ease-out
[[nodiscard]] inline constexpr float ease_out_cubic(float t) noexcept {
    float t1 = t - 1.0f;
    return t1 * t1 * t1 + 1.0f;
}

/// Cubic ease-in-out
[[nodiscard]] inline constexpr float ease_in_out_cubic(float t) noexcept {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    float t1 = 2.0f * t - 2.0f;
    return 0.5f * t1 * t1 * t1 + 1.0f;
}

/// Sine ease-in
[[nodiscard]] inline float ease_in_sine(float t) noexcept {
    return 1.0f - std::cos(t * std::numbers::pi_v<float> / 2.0f);
}

/// Sine ease-out
[[nodiscard]] inline float ease_out_sine(float t) noexcept {
    return std::sin(t * std::numbers::pi_v<float> / 2.0f);
}

/// Sine ease-in-out
[[nodiscard]] inline float ease_in_out_sine(float t) noexcept {
    return 0.5f * (1.0f - std::cos(std::numbers::pi_v<float> * t));
}

/// Exponential ease-in
[[nodiscard]] inline float ease_in_expo(float t) noexcept {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
}

/// Exponential ease-out
[[nodiscard]] inline float ease_out_expo(float t) noexcept {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

/// Exponential ease-in-out
[[nodiscard]] inline float ease_in_out_expo(float t) noexcept {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    if (t < 0.5f) {
        return 0.5f * std::pow(2.0f, 20.0f * t - 10.0f);
    }
    return 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
}

/// Back ease-in (overshoots)
[[nodiscard]] inline constexpr float ease_in_back(float t) noexcept {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

/// Back ease-out (overshoots)
[[nodiscard]] inline constexpr float ease_out_back(float t) noexcept {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    float t1 = t - 1.0f;
    return 1.0f + c3 * t1 * t1 * t1 + c1 * t1 * t1;
}

/// Back ease-in-out (overshoots)
[[nodiscard]] inline constexpr float ease_in_out_back(float t) noexcept {
    constexpr float c1 = 1.70158f;
    constexpr float c2 = c1 * 1.525f;
    if (t < 0.5f) {
        return 0.5f * (4.0f * t * t * ((c2 + 1.0f) * 2.0f * t - c2));
    }
    float t1 = 2.0f * t - 2.0f;
    return 0.5f * (t1 * t1 * ((c2 + 1.0f) * t1 + c2) + 2.0f);
}

/// Bounce ease-out helper
[[nodiscard]] inline constexpr float bounce_out_impl(float t) noexcept {
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;

    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        float t1 = t - 1.5f / d1;
        return n1 * t1 * t1 + 0.75f;
    } else if (t < 2.5f / d1) {
        float t1 = t - 2.25f / d1;
        return n1 * t1 * t1 + 0.9375f;
    } else {
        float t1 = t - 2.625f / d1;
        return n1 * t1 * t1 + 0.984375f;
    }
}

/// Bounce ease-in
[[nodiscard]] inline constexpr float ease_in_bounce(float t) noexcept {
    return 1.0f - bounce_out_impl(1.0f - t);
}

/// Bounce ease-out
[[nodiscard]] inline constexpr float ease_out_bounce(float t) noexcept {
    return bounce_out_impl(t);
}

/// Bounce ease-in-out
[[nodiscard]] inline constexpr float ease_in_out_bounce(float t) noexcept {
    return t < 0.5f
        ? 0.5f * (1.0f - bounce_out_impl(1.0f - 2.0f * t))
        : 0.5f * (1.0f + bounce_out_impl(2.0f * t - 1.0f));
}

/// Elastic ease-in
[[nodiscard]] inline float ease_in_elastic(float t) noexcept {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    constexpr float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
    return -std::pow(2.0f, 10.0f * t - 10.0f) *
           std::sin((t * 10.0f - 10.75f) * c4);
}

/// Elastic ease-out
[[nodiscard]] inline float ease_out_elastic(float t) noexcept {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    constexpr float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
    return std::pow(2.0f, -10.0f * t) *
           std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

/// Elastic ease-in-out
[[nodiscard]] inline float ease_in_out_elastic(float t) noexcept {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    constexpr float c5 = (2.0f * std::numbers::pi_v<float>) / 4.5f;
    if (t < 0.5f) {
        return -0.5f * std::pow(2.0f, 20.0f * t - 10.0f) *
               std::sin((20.0f * t - 11.125f) * c5);
    }
    return 0.5f * std::pow(2.0f, -20.0f * t + 10.0f) *
           std::sin((20.0f * t - 11.125f) * c5) + 1.0f;
}

/// Type alias for easing function pointer
using easing_function = float(*)(float) noexcept;

/**
 * @brief Get the easing function pointer for a type
 * @param type Easing type
 * @return Function pointer to the easing function
 *
 * This uses a lookup table for O(1) dispatch instead of a switch statement.
 * The function pointer can be cached for repeated use with the same easing type.
 *
 * @code
 * // Cache the function for repeated calls:
 * auto ease_fn = easing::get_function(easing_type::ease_out_quad);
 * for (int i = 0; i < 100; ++i) {
 *     float t = static_cast<float>(i) / 100.0f;
 *     float eased = ease_fn(t);  // Direct function call, no switch
 * }
 * @endcode
 */
[[nodiscard]] inline easing_function get_function(easing_type type) noexcept {
    // Lookup table indexed by easing_type enum value
    // This provides O(1) dispatch instead of O(n) switch
    static constexpr easing_function table[] = {
        linear,              // 0: linear
        ease_in_quad,        // 1: ease_in_quad
        ease_out_quad,       // 2: ease_out_quad
        ease_in_out_quad,    // 3: ease_in_out_quad
        ease_in_cubic,       // 4: ease_in_cubic
        ease_out_cubic,      // 5: ease_out_cubic
        ease_in_out_cubic,   // 6: ease_in_out_cubic
        ease_in_sine,        // 7: ease_in_sine
        ease_out_sine,       // 8: ease_out_sine
        ease_in_out_sine,    // 9: ease_in_out_sine
        ease_in_expo,        // 10: ease_in_expo
        ease_out_expo,       // 11: ease_out_expo
        ease_in_out_expo,    // 12: ease_in_out_expo
        ease_in_back,        // 13: ease_in_back
        ease_out_back,       // 14: ease_out_back
        ease_in_out_back,    // 15: ease_in_out_back
        ease_in_bounce,      // 16: ease_in_bounce
        ease_out_bounce,     // 17: ease_out_bounce
        ease_in_out_bounce,  // 18: ease_in_out_bounce
        ease_in_elastic,     // 19: ease_in_elastic
        ease_out_elastic,    // 20: ease_out_elastic
        ease_in_out_elastic, // 21: ease_in_out_elastic
    };

    auto index = static_cast<std::size_t>(type);
    constexpr std::size_t TABLE_SIZE = sizeof(table) / sizeof(table[0]);

    if (index >= TABLE_SIZE) {
        return linear;  // Fallback for invalid type
    }

    return table[index];
}

/**
 * @brief Apply easing function by type
 * @param type Easing type
 * @param t Progress value [0,1]
 * @return Eased value
 *
 * @note For repeated calls with the same easing type, consider caching
 * the function pointer via get_function() for better performance.
 */
[[nodiscard]] inline float apply(easing_type type, float t) noexcept {
    return get_function(type)(t);
}

} // namespace easing

// ============================================================================
// Animation Base
// ============================================================================

/**
 * @enum animation_state
 * @brief Current state of an animation
 */
enum class animation_state {
    pending,    ///< Not yet started
    running,    ///< Currently animating
    paused,     ///< Temporarily paused
    completed,  ///< Finished
    cancelled   ///< Stopped before completion
};

/**
 * @enum animation_direction
 * @brief Direction of animation playback
 */
enum class animation_direction {
    forward,    ///< Play from start to end
    reverse,    ///< Play from end to start
    alternate,  ///< Alternate forward/reverse each iteration
    alternate_reverse  ///< Start reverse, then alternate
};

/**
 * @class animation
 * @brief Base class for all animations
 */
class animation {
public:
    using completion_callback = std::function<void()>;
    using update_callback = std::function<void(float progress)>;

    virtual ~animation() = default;

    // ===== Configuration =====

    /**
     * @brief Set animation duration in milliseconds
     */
    void set_duration(unsigned int ms) noexcept {
        m_duration_ms = ms;
    }

    [[nodiscard]] unsigned int duration() const noexcept {
        return m_duration_ms;
    }

    /**
     * @brief Set delay before animation starts
     */
    void set_delay(unsigned int ms) noexcept {
        m_delay_ms = ms;
    }

    [[nodiscard]] unsigned int delay() const noexcept {
        return m_delay_ms;
    }

    /**
     * @brief Set number of iterations (0 = infinite)
     */
    void set_iterations(unsigned int count) noexcept {
        m_iterations = count;
    }

    [[nodiscard]] unsigned int iterations() const noexcept {
        return m_iterations;
    }

    /**
     * @brief Set easing function
     */
    void set_easing(easing_type type) noexcept {
        m_easing = type;
    }

    [[nodiscard]] easing_type get_easing() const noexcept {
        return m_easing;
    }

    /**
     * @brief Set playback direction
     */
    void set_direction(animation_direction dir) noexcept {
        m_direction = dir;
    }

    [[nodiscard]] animation_direction direction() const noexcept {
        return m_direction;
    }

    // ===== Callbacks =====

    /**
     * @brief Set callback for when animation completes
     */
    void set_on_complete(completion_callback callback) {
        m_on_complete = std::move(callback);
    }

    /**
     * @brief Set callback for each update
     */
    void set_on_update(update_callback callback) {
        m_on_update = std::move(callback);
    }

    // ===== State =====

    [[nodiscard]] animation_state state() const noexcept {
        return m_state;
    }

    [[nodiscard]] bool is_running() const noexcept {
        return m_state == animation_state::running;
    }

    [[nodiscard]] bool is_completed() const noexcept {
        return m_state == animation_state::completed;
    }

    [[nodiscard]] float progress() const noexcept {
        return m_progress;
    }

    [[nodiscard]] unsigned int current_iteration() const noexcept {
        return m_current_iteration;
    }

    // ===== Control =====

    /**
     * @brief Start the animation
     */
    void start() {
        if (m_state == animation_state::pending ||
            m_state == animation_state::completed ||
            m_state == animation_state::cancelled) {
            m_state = animation_state::running;
            m_elapsed_ms = 0;
            m_current_iteration = 0;
            m_progress = 0.0f;
            on_start();
        }
    }

    /**
     * @brief Pause the animation
     */
    void pause() {
        if (m_state == animation_state::running) {
            m_state = animation_state::paused;
        }
    }

    /**
     * @brief Resume a paused animation
     */
    void resume() {
        if (m_state == animation_state::paused) {
            m_state = animation_state::running;
        }
    }

    /**
     * @brief Stop the animation
     */
    void stop() {
        if (m_state == animation_state::running ||
            m_state == animation_state::paused) {
            m_state = animation_state::cancelled;
            on_stop();
        }
    }

    /**
     * @brief Reset animation to initial state
     */
    void reset() {
        m_state = animation_state::pending;
        m_elapsed_ms = 0;
        m_current_iteration = 0;
        m_progress = 0.0f;
        on_reset();
    }

    /**
     * @brief Update animation by delta time
     * @param delta_ms Time elapsed since last update in milliseconds
     * @return true if animation is still running
     *
     * @note Uses 64-bit elapsed time to prevent overflow during long-running
     *       animations. An animation can run for ~584 million years before
     *       overflow (vs ~49 days with 32-bit).
     */
    bool update(unsigned int delta_ms) {
        if (m_state != animation_state::running) {
            return false;
        }

        // Use 64-bit arithmetic to prevent overflow
        // Saturate at max value rather than wrapping (theoretical limit: 584 million years)
        constexpr uint64_t MAX_ELAPSED = UINT64_MAX - 0xFFFFFFFF;  // Leave room for max delta
        if (m_elapsed_ms < MAX_ELAPSED) {
            m_elapsed_ms += delta_ms;
        }

        // Handle delay
        if (m_elapsed_ms < m_delay_ms) {
            return true;
        }

        uint64_t animation_time = m_elapsed_ms - m_delay_ms;

        // Calculate progress within current iteration
        if (m_duration_ms == 0) {
            m_progress = 1.0f;
        } else {
            uint64_t iteration_time = animation_time % m_duration_ms;
            m_current_iteration = static_cast<unsigned int>(
                (animation_time / m_duration_ms) < UINT32_MAX
                    ? (animation_time / m_duration_ms)
                    : UINT32_MAX);

            // Check if we've completed all iterations
            if (m_iterations > 0 && m_current_iteration >= m_iterations) {
                m_progress = 1.0f;
                m_state = animation_state::completed;
                apply_progress(get_final_progress());
                if (m_on_complete) {
                    m_on_complete();
                }
                return false;
            }

            // Safe cast: iteration_time is always < m_duration_ms (from modulo)
            float raw_progress = static_cast<float>(iteration_time) /
                                 static_cast<float>(m_duration_ms);
            m_progress = calculate_directed_progress(raw_progress);
        }

        // Apply easing
        float eased_progress = easing::apply(m_easing, m_progress);
        apply_progress(eased_progress);

        if (m_on_update) {
            m_on_update(eased_progress);
        }

        return true;
    }

protected:
    /**
     * @brief Apply animation at given progress
     * @param progress Eased progress value [0,1]
     */
    virtual void apply_progress(float progress) = 0;

    /**
     * @brief Called when animation starts
     */
    virtual void on_start() {}

    /**
     * @brief Called when animation stops
     */
    virtual void on_stop() {}

    /**
     * @brief Called when animation resets
     */
    virtual void on_reset() {}

private:
    unsigned int m_duration_ms = 300;     ///< Duration per iteration (ms)
    unsigned int m_delay_ms = 0;          ///< Delay before starting (ms)
    unsigned int m_iterations = 1;        ///< Number of iterations (0 = infinite)
    easing_type m_easing = easing_type::ease_out_quad;
    animation_direction m_direction = animation_direction::forward;
    animation_state m_state = animation_state::pending;
    uint64_t m_elapsed_ms = 0;            ///< Total elapsed time (64-bit to prevent overflow)
    unsigned int m_current_iteration = 0; ///< Current iteration number
    float m_progress = 0.0f;              ///< Progress within current iteration [0,1]
    completion_callback m_on_complete;
    update_callback m_on_update;

    [[nodiscard]] float calculate_directed_progress(float raw) const noexcept {
        switch (m_direction) {
            case animation_direction::forward:
                return raw;
            case animation_direction::reverse:
                return 1.0f - raw;
            case animation_direction::alternate:
                return (m_current_iteration % 2 == 0) ? raw : 1.0f - raw;
            case animation_direction::alternate_reverse:
                return (m_current_iteration % 2 == 0) ? 1.0f - raw : raw;
            default:
                return raw;
        }
    }

    [[nodiscard]] float get_final_progress() const noexcept {
        switch (m_direction) {
            case animation_direction::forward:
                return 1.0f;
            case animation_direction::reverse:
                return 0.0f;
            case animation_direction::alternate:
                return (m_iterations % 2 == 0) ? 0.0f : 1.0f;
            case animation_direction::alternate_reverse:
                return (m_iterations % 2 == 0) ? 1.0f : 0.0f;
            default:
                return 1.0f;
        }
    }
};

// ============================================================================
// Concrete Animation Types
// ============================================================================

/**
 * @class float_animation
 * @brief Animates a float value between two endpoints
 */
class float_animation : public animation {
public:
    float_animation(float from, float to)
        : m_from(from), m_to(to), m_current(from) {}

    void set_range(float from, float to) noexcept {
        m_from = from;
        m_to = to;
    }

    [[nodiscard]] float value() const noexcept {
        return m_current;
    }

    [[nodiscard]] float from() const noexcept { return m_from; }
    [[nodiscard]] float to() const noexcept { return m_to; }

protected:
    void apply_progress(float progress) override {
        m_current = m_from + (m_to - m_from) * progress;
    }

    void on_reset() override {
        m_current = m_from;
    }

private:
    float m_from;
    float m_to;
    float m_current;
};

/**
 * @class int_animation
 * @brief Animates an integer value between two endpoints
 */
class int_animation : public animation {
public:
    int_animation(int from, int to)
        : m_from(from), m_to(to), m_current(from) {}

    void set_range(int from, int to) noexcept {
        m_from = from;
        m_to = to;
    }

    [[nodiscard]] int value() const noexcept {
        return m_current;
    }

    [[nodiscard]] int from() const noexcept { return m_from; }
    [[nodiscard]] int to() const noexcept { return m_to; }

protected:
    void apply_progress(float progress) override {
        m_current = m_from + static_cast<int>(
            static_cast<float>(m_to - m_from) * progress + 0.5f);
    }

    void on_reset() override {
        m_current = m_from;
    }

private:
    int m_from;
    int m_to;
    int m_current;
};

/**
 * @class frame_animation
 * @brief Cycles through a sequence of tile IDs for sprite animation
 */
class frame_animation : public animation {
public:
    frame_animation() = default;

    explicit frame_animation(std::vector<int> frames)
        : m_frames(std::move(frames)) {
        if (!m_frames.empty()) {
            m_current_frame = m_frames[0];
        }
    }

    /**
     * @brief Set frames from a range of consecutive tile IDs
     * @param first_tile First tile ID
     * @param count Number of frames
     */
    void set_frame_range(int first_tile, int count) {
        m_frames.clear();
        m_frames.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            m_frames.push_back(first_tile + i);
        }
        if (!m_frames.empty()) {
            m_current_frame = m_frames[0];
        }
    }

    /**
     * @brief Set explicit frame sequence
     */
    void set_frames(std::vector<int> frames) {
        m_frames = std::move(frames);
        if (!m_frames.empty()) {
            m_current_frame = m_frames[0];
        }
    }

    /**
     * @brief Get current frame tile ID
     */
    [[nodiscard]] int current_frame() const noexcept {
        return m_current_frame;
    }

    /**
     * @brief Get frame count
     */
    [[nodiscard]] std::size_t frame_count() const noexcept {
        return m_frames.size();
    }

protected:
    void apply_progress(float progress) override {
        if (m_frames.empty()) return;

        // Map progress to frame index
        std::size_t frame_index = static_cast<std::size_t>(
            progress * static_cast<float>(m_frames.size()));

        // Clamp to valid range
        if (frame_index >= m_frames.size()) {
            frame_index = m_frames.size() - 1;
        }

        m_current_frame = m_frames[frame_index];
    }

    void on_reset() override {
        if (!m_frames.empty()) {
            m_current_frame = m_frames[0];
        }
    }

private:
    std::vector<int> m_frames;
    int m_current_frame = -1;
};

/**
 * @class position_animation
 * @brief Animates x,y position
 */
class position_animation : public animation {
public:
    position_animation(int from_x, int from_y, int to_x, int to_y)
        : m_from_x(from_x), m_from_y(from_y)
        , m_to_x(to_x), m_to_y(to_y)
        , m_current_x(from_x), m_current_y(from_y) {}

    void set_from(int x, int y) noexcept {
        m_from_x = x;
        m_from_y = y;
    }

    void set_to(int x, int y) noexcept {
        m_to_x = x;
        m_to_y = y;
    }

    [[nodiscard]] int x() const noexcept { return m_current_x; }
    [[nodiscard]] int y() const noexcept { return m_current_y; }

protected:
    void apply_progress(float progress) override {
        m_current_x = m_from_x + static_cast<int>(
            static_cast<float>(m_to_x - m_from_x) * progress + 0.5f);
        m_current_y = m_from_y + static_cast<int>(
            static_cast<float>(m_to_y - m_from_y) * progress + 0.5f);
    }

    void on_reset() override {
        m_current_x = m_from_x;
        m_current_y = m_from_y;
    }

private:
    int m_from_x, m_from_y;
    int m_to_x, m_to_y;
    int m_current_x, m_current_y;
};

/**
 * @class color_animation
 * @brief Animates RGBA color values
 */
class color_animation : public animation {
public:
    struct rgba {
        uint8_t r = 255, g = 255, b = 255, a = 255;
    };

    color_animation(rgba from, rgba to)
        : m_from(from), m_to(to), m_current(from) {}

    void set_from(rgba color) noexcept { m_from = color; }
    void set_to(rgba color) noexcept { m_to = color; }

    [[nodiscard]] rgba value() const noexcept { return m_current; }
    [[nodiscard]] uint8_t r() const noexcept { return m_current.r; }
    [[nodiscard]] uint8_t g() const noexcept { return m_current.g; }
    [[nodiscard]] uint8_t b() const noexcept { return m_current.b; }
    [[nodiscard]] uint8_t a() const noexcept { return m_current.a; }

protected:
    void apply_progress(float progress) override {
        m_current.r = lerp_channel(m_from.r, m_to.r, progress);
        m_current.g = lerp_channel(m_from.g, m_to.g, progress);
        m_current.b = lerp_channel(m_from.b, m_to.b, progress);
        m_current.a = lerp_channel(m_from.a, m_to.a, progress);
    }

    void on_reset() override {
        m_current = m_from;
    }

private:
    rgba m_from, m_to, m_current;

    static uint8_t lerp_channel(uint8_t from, uint8_t to, float t) noexcept {
        return static_cast<uint8_t>(
            static_cast<float>(from) +
            (static_cast<float>(to) - static_cast<float>(from)) * t + 0.5f);
    }
};

// ============================================================================
// Animation Controller
// ============================================================================

/**
 * @class animation_controller
 * @brief Manages multiple animations
 *
 * Updates all active animations and removes completed ones.
 */
class animation_controller {
public:
    using animation_ptr = std::shared_ptr<animation>;

    /**
     * @brief Add an animation to be managed
     * @param anim Animation to add
     * @param auto_start If true, starts the animation immediately
     * @return Pointer to the added animation
     */
    animation_ptr add(animation_ptr anim, bool auto_start = true) {
        if (auto_start) {
            anim->start();
        }
        m_animations.push_back(anim);
        return anim;
    }

    /**
     * @brief Create and add a float animation
     */
    std::shared_ptr<float_animation> add_float(
        float from, float to,
        unsigned int duration_ms = 300,
        easing_type easing = easing_type::ease_out_quad) {
        auto anim = std::make_shared<float_animation>(from, to);
        anim->set_duration(duration_ms);
        anim->set_easing(easing);
        anim->start();
        m_animations.push_back(anim);
        return anim;
    }

    /**
     * @brief Create and add a frame animation
     */
    std::shared_ptr<frame_animation> add_frames(
        int first_tile, int count,
        unsigned int duration_ms = 500,
        unsigned int iterations = 0) {
        auto anim = std::make_shared<frame_animation>();
        anim->set_frame_range(first_tile, count);
        anim->set_duration(duration_ms);
        anim->set_iterations(iterations);
        anim->set_easing(easing_type::linear);
        anim->start();
        m_animations.push_back(anim);
        return anim;
    }

    /**
     * @brief Create and add a position animation
     */
    std::shared_ptr<position_animation> add_position(
        int from_x, int from_y, int to_x, int to_y,
        unsigned int duration_ms = 300,
        easing_type easing = easing_type::ease_out_quad) {
        auto anim = std::make_shared<position_animation>(
            from_x, from_y, to_x, to_y);
        anim->set_duration(duration_ms);
        anim->set_easing(easing);
        anim->start();
        m_animations.push_back(anim);
        return anim;
    }

    /**
     * @brief Update all animations
     * @param delta_ms Time elapsed since last update
     */
    void update(unsigned int delta_ms) {
        // Update all animations
        for (auto& anim : m_animations) {
            anim->update(delta_ms);
        }

        // Remove completed/cancelled animations
        m_animations.erase(
            std::remove_if(m_animations.begin(), m_animations.end(),
                [](const animation_ptr& a) {
                    return a->state() == animation_state::completed ||
                           a->state() == animation_state::cancelled;
                }),
            m_animations.end());
    }

    /**
     * @brief Stop and remove all animations
     */
    void clear() {
        for (auto& anim : m_animations) {
            anim->stop();
        }
        m_animations.clear();
    }

    /**
     * @brief Get number of active animations
     */
    [[nodiscard]] std::size_t count() const noexcept {
        return m_animations.size();
    }

    /**
     * @brief Check if any animations are running
     */
    [[nodiscard]] bool has_animations() const noexcept {
        return !m_animations.empty();
    }

    /**
     * @brief Pause all animations
     */
    void pause_all() {
        for (auto& anim : m_animations) {
            anim->pause();
        }
    }

    /**
     * @brief Resume all paused animations
     */
    void resume_all() {
        for (auto& anim : m_animations) {
            anim->resume();
        }
    }

private:
    std::vector<animation_ptr> m_animations;
};

// ============================================================================
// Convenience Functions
// ============================================================================

/**
 * @brief Linear interpolation helper
 */
template<typename T>
[[nodiscard]] constexpr T lerp(T a, T b, float t) noexcept {
    return a + static_cast<T>((b - a) * t);
}

} // namespace onyxui::tile
