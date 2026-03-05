/**
 * @file test_tile_animation.cc
 * @brief Unit tests for tile animation system
 */

#include <doctest/doctest.h>
#include <onyxui/tile/tile_animation.hh>
#include <cmath>

using namespace onyxui::tile;

// ============================================================================
// Easing Function Tests
// ============================================================================

TEST_SUITE("easing_functions") {
    TEST_CASE("Linear easing") {
        CHECK(easing::linear(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::linear(0.5f) == doctest::Approx(0.5f));
        CHECK(easing::linear(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Quadratic easing boundaries") {
        // All easing functions should return 0 at t=0 and 1 at t=1
        CHECK(easing::ease_in_quad(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_quad(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_out_quad(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_out_quad(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_in_out_quad(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_out_quad(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Cubic easing boundaries") {
        CHECK(easing::ease_in_cubic(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_cubic(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_out_cubic(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_out_cubic(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_in_out_cubic(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_out_cubic(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Sine easing boundaries") {
        CHECK(easing::ease_in_sine(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_sine(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_out_sine(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_out_sine(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_in_out_sine(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_out_sine(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Expo easing boundaries") {
        CHECK(easing::ease_in_expo(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_expo(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_out_expo(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_out_expo(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_in_out_expo(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_out_expo(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Back easing boundaries") {
        CHECK(easing::ease_in_back(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_back(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_out_back(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_out_back(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_in_out_back(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_out_back(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Bounce easing boundaries") {
        CHECK(easing::ease_in_bounce(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_bounce(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_out_bounce(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_out_bounce(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_in_out_bounce(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_out_bounce(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Elastic easing boundaries") {
        CHECK(easing::ease_in_elastic(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_elastic(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_out_elastic(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_out_elastic(1.0f) == doctest::Approx(1.0f));
        CHECK(easing::ease_in_out_elastic(0.0f) == doctest::Approx(0.0f));
        CHECK(easing::ease_in_out_elastic(1.0f) == doctest::Approx(1.0f));
    }

    TEST_CASE("Ease-in vs ease-out comparison") {
        // ease-in should be slower at start (lower value at t=0.5)
        // ease-out should be faster at start (higher value at t=0.5)
        float ease_in = easing::ease_in_quad(0.5f);
        float ease_out = easing::ease_out_quad(0.5f);

        CHECK(ease_in < 0.5f);
        CHECK(ease_out > 0.5f);
    }

    TEST_CASE("Apply function dispatches correctly") {
        CHECK(easing::apply(easing_type::linear, 0.5f) == doctest::Approx(0.5f));
        CHECK(easing::apply(easing_type::ease_in_quad, 0.5f) ==
              doctest::Approx(easing::ease_in_quad(0.5f)));
        CHECK(easing::apply(easing_type::ease_out_bounce, 0.5f) ==
              doctest::Approx(easing::ease_out_bounce(0.5f)));
    }
}

// ============================================================================
// Float Animation Tests
// ============================================================================

TEST_SUITE("float_animation") {
    TEST_CASE("Default state") {
        float_animation anim(0.0f, 100.0f);

        CHECK(anim.state() == animation_state::pending);
        CHECK(anim.value() == doctest::Approx(0.0f));
        CHECK(anim.from() == doctest::Approx(0.0f));
        CHECK(anim.to() == doctest::Approx(100.0f));
    }

    TEST_CASE("Animation runs to completion") {
        float_animation anim(0.0f, 100.0f);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();
        CHECK(anim.is_running());

        // Update halfway
        anim.update(50);
        CHECK(anim.value() == doctest::Approx(50.0f));

        // Update to completion
        anim.update(50);
        CHECK(anim.is_completed());
        CHECK(anim.value() == doctest::Approx(100.0f));
    }

    TEST_CASE("Animation with delay") {
        float_animation anim(0.0f, 100.0f);
        anim.set_duration(100);
        anim.set_delay(50);
        anim.set_easing(easing_type::linear);

        anim.start();

        // During delay, value should not change
        anim.update(25);
        CHECK(anim.value() == doctest::Approx(0.0f));

        // Past delay, animation starts
        anim.update(75);  // 25 more delay + 50 animation
        CHECK(anim.value() == doctest::Approx(50.0f));
    }

    TEST_CASE("Animation reset") {
        float_animation anim(10.0f, 50.0f);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(100);
        CHECK(anim.is_completed());
        CHECK(anim.value() == doctest::Approx(50.0f));

        anim.reset();
        CHECK(anim.state() == animation_state::pending);
        CHECK(anim.value() == doctest::Approx(10.0f));
    }

    TEST_CASE("Completion callback") {
        float_animation anim(0.0f, 1.0f);
        anim.set_duration(100);

        bool completed = false;
        anim.set_on_complete([&completed]() {
            completed = true;
        });

        anim.start();
        anim.update(100);

        CHECK(completed);
    }

    TEST_CASE("Update callback") {
        float_animation anim(0.0f, 1.0f);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        float last_progress = -1.0f;
        anim.set_on_update([&last_progress](float progress) {
            last_progress = progress;
        });

        anim.start();
        anim.update(50);

        CHECK(last_progress == doctest::Approx(0.5f));
    }
}

// ============================================================================
// Int Animation Tests
// ============================================================================

TEST_SUITE("int_animation") {
    TEST_CASE("Integer interpolation") {
        int_animation anim(0, 100);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(50);

        CHECK(anim.value() == 50);
    }

    TEST_CASE("Integer rounding") {
        int_animation anim(0, 10);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();

        // At 25%, value should be ~2.5, rounded to 3
        anim.update(25);
        CHECK(anim.value() == 3);
    }
}

// ============================================================================
// Frame Animation Tests
// ============================================================================

TEST_SUITE("frame_animation") {
    TEST_CASE("Default construction") {
        frame_animation anim;

        CHECK(anim.frame_count() == 0);
        CHECK(anim.current_frame() == -1);
    }

    TEST_CASE("Set frame range") {
        frame_animation anim;
        anim.set_frame_range(100, 4);

        CHECK(anim.frame_count() == 4);
        CHECK(anim.current_frame() == 100);  // First frame
    }

    TEST_CASE("Frame cycling") {
        frame_animation anim;
        anim.set_frame_range(0, 4);  // Frames 0, 1, 2, 3
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();

        // At start
        CHECK(anim.current_frame() == 0);

        // At 25%
        anim.update(25);
        CHECK(anim.current_frame() == 1);

        // At 50%
        anim.update(25);
        CHECK(anim.current_frame() == 2);

        // At 75%
        anim.update(25);
        CHECK(anim.current_frame() == 3);
    }

    TEST_CASE("Looping frame animation") {
        frame_animation anim;
        anim.set_frame_range(0, 2);
        anim.set_duration(100);
        anim.set_iterations(0);  // Infinite
        anim.set_easing(easing_type::linear);

        anim.start();

        // First cycle
        anim.update(100);
        CHECK(anim.is_running());  // Still running (infinite)

        // Second cycle
        anim.update(50);
        CHECK(anim.current_frame() == 1);
    }

    TEST_CASE("Custom frame sequence") {
        frame_animation anim;
        anim.set_frames({5, 10, 15, 10, 5});  // Ping-pong sequence
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        CHECK(anim.frame_count() == 5);
        CHECK(anim.current_frame() == 5);
    }
}

// ============================================================================
// Position Animation Tests
// ============================================================================

TEST_SUITE("position_animation") {
    TEST_CASE("Position interpolation") {
        position_animation anim(0, 0, 100, 50);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(50);

        CHECK(anim.x() == 50);
        CHECK(anim.y() == 25);
    }

    TEST_CASE("Position at completion") {
        position_animation anim(10, 20, 30, 40);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(100);

        CHECK(anim.x() == 30);
        CHECK(anim.y() == 40);
    }
}

// ============================================================================
// Color Animation Tests
// ============================================================================

TEST_SUITE("color_animation") {
    TEST_CASE("Color interpolation") {
        color_animation::rgba from{0, 0, 0, 255};
        color_animation::rgba to{255, 128, 64, 128};

        color_animation anim(from, to);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(50);

        CHECK(anim.r() == 128);  // ~127.5 rounded
        CHECK(anim.g() == 64);
        CHECK(anim.b() == 32);
        CHECK(anim.a() == 192);  // 255 - 63.5
    }
}

// ============================================================================
// Animation Direction Tests
// ============================================================================

TEST_SUITE("animation_direction") {
    TEST_CASE("Forward direction") {
        float_animation anim(0.0f, 100.0f);
        anim.set_duration(100);
        anim.set_direction(animation_direction::forward);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(50);

        CHECK(anim.value() == doctest::Approx(50.0f));
    }

    TEST_CASE("Reverse direction") {
        float_animation anim(0.0f, 100.0f);
        anim.set_duration(100);
        anim.set_direction(animation_direction::reverse);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(50);

        CHECK(anim.value() == doctest::Approx(50.0f));  // 100 - 50
    }

    TEST_CASE("Alternate direction") {
        float_animation anim(0.0f, 100.0f);
        anim.set_duration(100);
        anim.set_iterations(2);
        anim.set_direction(animation_direction::alternate);
        anim.set_easing(easing_type::linear);

        anim.start();

        // First iteration: forward
        anim.update(50);
        CHECK(anim.value() == doctest::Approx(50.0f));

        // Complete first iteration
        anim.update(50);
        CHECK(anim.current_iteration() == 1);

        // Second iteration: reverse
        anim.update(50);
        CHECK(anim.value() == doctest::Approx(50.0f));  // Going back
    }
}

// ============================================================================
// Animation Controller Tests
// ============================================================================

TEST_SUITE("animation_controller") {
    TEST_CASE("Add and update animations") {
        animation_controller controller;

        auto anim = controller.add_float(0.0f, 100.0f, 100);

        CHECK(controller.count() == 1);
        CHECK(controller.has_animations());

        controller.update(100);

        CHECK(controller.count() == 0);  // Completed, removed
        CHECK_FALSE(controller.has_animations());
    }

    TEST_CASE("Multiple animations") {
        animation_controller controller;

        auto anim1 = controller.add_float(0.0f, 100.0f, 100);
        auto anim2 = controller.add_float(0.0f, 50.0f, 200);

        CHECK(controller.count() == 2);

        controller.update(100);
        CHECK(controller.count() == 1);  // First completed

        controller.update(100);
        CHECK(controller.count() == 0);  // Both completed
    }

    TEST_CASE("Clear animations") {
        animation_controller controller;

        controller.add_float(0.0f, 100.0f, 1000);
        controller.add_float(0.0f, 100.0f, 1000);
        controller.add_float(0.0f, 100.0f, 1000);

        CHECK(controller.count() == 3);

        controller.clear();

        CHECK(controller.count() == 0);
    }

    TEST_CASE("Pause and resume all") {
        animation_controller controller;

        auto anim = controller.add_float(0.0f, 100.0f, 100);
        anim->set_easing(easing_type::linear);

        controller.update(50);
        CHECK(anim->value() == doctest::Approx(50.0f));

        controller.pause_all();
        controller.update(25);  // Should not advance
        CHECK(anim->value() == doctest::Approx(50.0f));

        controller.resume_all();
        controller.update(25);
        CHECK(anim->value() == doctest::Approx(75.0f));
    }

    TEST_CASE("Add frame animation helper") {
        animation_controller controller;

        auto anim = controller.add_frames(100, 4, 400, 0);

        CHECK(anim->frame_count() == 4);
        CHECK(anim->current_frame() == 100);
        CHECK(anim->iterations() == 0);  // Infinite
    }

    TEST_CASE("Add position animation helper") {
        animation_controller controller;

        auto anim = controller.add_position(0, 0, 100, 100, 100);
        anim->set_easing(easing_type::linear);

        controller.update(50);

        CHECK(anim->x() == 50);
        CHECK(anim->y() == 50);
    }
}

// ============================================================================
// Animation State Tests
// ============================================================================

TEST_SUITE("animation_state") {
    TEST_CASE("State transitions") {
        float_animation anim(0.0f, 1.0f);
        anim.set_duration(100);

        CHECK(anim.state() == animation_state::pending);

        anim.start();
        CHECK(anim.state() == animation_state::running);

        anim.pause();
        CHECK(anim.state() == animation_state::paused);

        anim.resume();
        CHECK(anim.state() == animation_state::running);

        anim.stop();
        CHECK(anim.state() == animation_state::cancelled);
    }

    TEST_CASE("Pause prevents update") {
        float_animation anim(0.0f, 100.0f);
        anim.set_duration(100);
        anim.set_easing(easing_type::linear);

        anim.start();
        anim.update(25);
        CHECK(anim.value() == doctest::Approx(25.0f));

        anim.pause();
        anim.update(50);  // Should not advance
        CHECK(anim.value() == doctest::Approx(25.0f));

        anim.resume();
        anim.update(25);
        CHECK(anim.value() == doctest::Approx(50.0f));
    }

    TEST_CASE("Restart after completion") {
        float_animation anim(0.0f, 100.0f);
        anim.set_duration(100);

        anim.start();
        anim.update(100);
        CHECK(anim.is_completed());

        anim.start();  // Restart
        CHECK(anim.is_running());
        CHECK(anim.progress() == doctest::Approx(0.0f));
    }
}

// ============================================================================
// Lerp Helper Tests
// ============================================================================

TEST_SUITE("lerp_helper") {
    TEST_CASE("Integer lerp") {
        CHECK(lerp(0, 100, 0.0f) == 0);
        CHECK(lerp(0, 100, 0.5f) == 50);
        CHECK(lerp(0, 100, 1.0f) == 100);
    }

    TEST_CASE("Float lerp") {
        CHECK(lerp(0.0f, 100.0f, 0.0f) == doctest::Approx(0.0f));
        CHECK(lerp(0.0f, 100.0f, 0.5f) == doctest::Approx(50.0f));
        CHECK(lerp(0.0f, 100.0f, 1.0f) == doctest::Approx(100.0f));
    }
}
