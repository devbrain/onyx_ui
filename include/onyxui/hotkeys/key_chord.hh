/**
 * @file key_chord.hh
 * @brief Support for multi-key sequences (chords) and single-key activation
 * @author igor
 * @date 2025-10-27
 *
 * @details
 * Extends the basic key_sequence to support:
 * - Multi-key sequences (Emacs-style): Ctrl+X, Ctrl+C
 * - Single-key activation (QBasic-style): Alt key alone activates menu
 * - Vim-style sequences: gg, dd, etc.
 *
 * ## Design Philosophy
 *
 * Different UI paradigms require different hotkey behaviors:
 *
 * **Single-Key Activation** (MS-DOS apps):
 * - Pressing Alt alone activates menu bar
 * - No need to release and press another key
 * - Immediate feedback on keydown
 *
 * **Multi-Key Sequences** (Emacs):
 * - Ctrl+X followed by Ctrl+C to exit
 * - State machine tracks partial sequences
 * - Timeout cancels incomplete sequences
 *
 * **Modal Sequences** (Vim):
 * - gg goes to top, dd deletes line
 * - No modifiers required
 * - Context-sensitive based on mode
 */

#pragma once

#include <vector>
#include <chrono>
#include <optional>
#include <onyxui/hotkeys/key_sequence.hh>

namespace onyxui {

    /**
     * @enum chord_type
     * @brief Type of key chord/sequence
     */
    enum class chord_type : uint8_t {
        single_key,      ///< Single key press (normal)
        modifier_only,   ///< Modifier key alone (Alt for menu)
        multi_sequence,  ///< Multiple keys in sequence (Ctrl+X, Ctrl+C)
        modal_sequence   ///< Modal sequence (gg, dd) - no modifiers
    };

    /**
     * @struct key_chord
     * @brief Represents a complex key sequence or chord
     *
     * @details
     * Extends key_sequence to support:
     * - Single modifier key activation (Alt alone)
     * - Multi-key sequences with timeout
     * - Modal sequences without modifiers
     */
    struct key_chord {
        chord_type type = chord_type::single_key;
        std::vector<key_sequence> sequence;  ///< The key sequence(s)
        std::chrono::milliseconds timeout_ms{1000};  ///< Timeout for multi-key sequences

        /**
         * @brief Default constructor - empty chord
         */
        key_chord() = default;

        /**
         * @brief Construct from single key_sequence
         * @param seq Single key sequence
         */
        explicit key_chord(const key_sequence& seq)
            : type(chord_type::single_key)
            , sequence{seq} {}

        /**
         * @brief Construct modifier-only chord (e.g., Alt for menu)
         * @param mod Modifier key that triggers action
         */
        explicit key_chord(key_modifier mod)
            : type(chord_type::modifier_only)
            , sequence{{'\0', mod}} {}

        /**
         * @brief Construct multi-key sequence
         * @param seq_list List of key sequences to press in order
         * @param timeout Timeout between keys (default 1000ms)
         */
        key_chord(std::initializer_list<key_sequence> seq_list,
                  std::chrono::milliseconds timeout = std::chrono::milliseconds{1000})
            : type(chord_type::multi_sequence)
            , sequence(seq_list)
            , timeout_ms(timeout) {}

        /**
         * @brief Check if this is a modifier-only chord
         */
        [[nodiscard]] bool is_modifier_only() const noexcept {
            return type == chord_type::modifier_only;
        }

        /**
         * @brief Check if this is a multi-key sequence
         */
        [[nodiscard]] bool is_multi_sequence() const noexcept {
            return type == chord_type::multi_sequence;
        }

        /**
         * @brief Get the length of the sequence
         */
        [[nodiscard]] size_t length() const noexcept {
            return sequence.size();
        }

        /**
         * @brief Check if chord is empty
         */
        [[nodiscard]] bool empty() const noexcept {
            return sequence.empty();
        }

        /**
         * @brief Comparison operator for use in std::map
         */
        auto operator<=>(const key_chord&) const = default;
    };

    /**
     * @brief Create an Emacs-style chord (e.g., Ctrl+X, Ctrl+C)
     * @param keys List of key sequences
     * @return key_chord configured for Emacs-style
     */
    inline key_chord make_emacs_chord(std::initializer_list<key_sequence> keys) {
        return key_chord(keys, std::chrono::milliseconds{1000});
    }

    /**
     * @brief Create a Vim-style chord (e.g., gg, dd)
     * @param keys String of keys to press
     * @return key_chord configured for Vim-style
     */
    inline key_chord make_vim_chord(const std::string& keys) {
        std::vector<key_sequence> seq;
        for (char c : keys) {
            seq.push_back(key_sequence{c});
        }
        key_chord chord;
        chord.type = chord_type::modal_sequence;
        chord.sequence = seq;
        chord.timeout_ms = std::chrono::milliseconds{500};  // Shorter timeout for modal
        return chord;
    }

    /**
     * @brief Create a QBasic-style modifier-only chord (Alt activates menu)
     * @param mod Modifier key
     * @return key_chord for modifier-only activation
     */
    inline key_chord make_modifier_chord(key_modifier mod) {
        return key_chord(mod);
    }

    /**
     * @class chord_matcher
     * @brief State machine for matching multi-key sequences
     *
     * @details
     * Tracks partial matches for multi-key sequences with timeout support.
     * Used internally by hotkey_manager to handle complex chords.
     */
    class chord_matcher {
    public:
        /**
         * @brief Reset the matcher state
         */
        void reset() {
            m_current_position = 0;
            m_last_key_time = {};
            m_active_chord = nullptr;
        }

        /**
         * @brief Process a key event for chord matching
         * @param seq The key sequence from current event
         * @param chord The chord to match against
         * @return true if chord is complete, false if partial/no match
         */
        bool process_key(const key_sequence& seq, const key_chord& chord) {
            auto now = std::chrono::steady_clock::now();

            // Check timeout for multi-key sequences
            if (m_active_chord && m_active_chord->is_multi_sequence()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_last_key_time);
                if (elapsed > m_active_chord->timeout_ms) {
                    reset();  // Timeout - start over
                }
            }

            // Start new chord or continue existing
            if (!m_active_chord) {
                m_active_chord = &chord;
                m_current_position = 0;
            }

            // Check if current key matches
            if (m_active_chord == &chord &&
                m_current_position < chord.sequence.size() &&
                chord.sequence[m_current_position] == seq) {

                m_current_position++;
                m_last_key_time = now;

                // Check if chord is complete
                if (m_current_position >= chord.sequence.size()) {
                    reset();
                    return true;  // Chord matched!
                }
            } else {
                // No match - reset
                reset();
            }

            return false;  // Partial match or no match
        }

        /**
         * @brief Check if a partial match is in progress
         */
        [[nodiscard]] bool has_partial_match() const noexcept {
            return m_active_chord != nullptr && m_current_position > 0;
        }

        /**
         * @brief Get the current position in the active chord
         */
        [[nodiscard]] size_t get_position() const noexcept {
            return m_current_position;
        }

    private:
        size_t m_current_position = 0;
        std::chrono::steady_clock::time_point m_last_key_time;
        const key_chord* m_active_chord = nullptr;
    };

} // namespace onyxui