/**
 * @file stack_panel.hh
 * @brief Chromeless stacked container — shows one child at a time
 */

#pragma once

#include <onyxui/core/element.hh>
#include <onyxui/core/signal.hh>
#include <algorithm>

namespace onyxui {

    template<UIBackend Backend>
    class stack_panel : public ui_element<Backend> {
    public:
        using base = ui_element<Backend>;
        using ui_element_ptr = typename base::ui_element_ptr;

        explicit stack_panel(ui_element<Backend>* parent = nullptr)
            : base(parent)
        {
            this->set_focusable(false);
        }
        ~stack_panel() override = default;

        stack_panel(const stack_panel&) = delete;
        stack_panel& operator=(const stack_panel&) = delete;
        stack_panel(stack_panel&&) noexcept = default;
        stack_panel& operator=(stack_panel&&) noexcept = default;

        // --- Page management ---

        int add_page(ui_element_ptr widget) {
            auto index = static_cast<int>(this->children().size());
            widget->set_visible(index == 0);
            this->add_child(std::move(widget));
            if (m_current_index < 0) {
                m_current_index = 0;
            }
            return index;
        }

        template<template<UIBackend> class W, typename... Args>
        auto* emplace_page(Args&&... args) {
            auto w = std::make_unique<W<Backend>>(std::forward<Args>(args)...);
            auto* ptr = w.get();
            add_page(std::move(w));
            return ptr;
        }

        void remove_page(int index) {
            auto& ch = this->children();
            if (index < 0 || index >= static_cast<int>(ch.size())) return;

            (void)this->remove_child(ch[static_cast<std::size_t>(index)].get());

            auto count = static_cast<int>(this->children().size());
            if (count == 0) {
                m_current_index = -1;
            } else if (m_current_index >= count) {
                m_current_index = count - 1;
                update_visibility();
            } else if (m_current_index > index) {
                m_current_index--;
            }
            this->invalidate_measure();
        }

        // --- Selection ---

        void set_current_index(int index) {
            auto count = static_cast<int>(this->children().size());
            if (count == 0) return;
            index = std::clamp(index, 0, count - 1);
            if (index == m_current_index) return;
            m_current_index = index;
            update_visibility();
            this->invalidate_measure();
            current_changed.emit(m_current_index);
        }

        [[nodiscard]] int current_index() const noexcept { return m_current_index; }

        [[nodiscard]] ui_element<Backend>* current_widget() const noexcept {
            auto& ch = this->children();
            if (m_current_index < 0 || m_current_index >= static_cast<int>(ch.size()))
                return nullptr;
            return ch[static_cast<std::size_t>(m_current_index)].get();
        }

        [[nodiscard]] int count() const noexcept {
            return static_cast<int>(this->children().size());
        }

        // --- Signal ---
        signal<int> current_changed;

    protected:
        logical_size do_measure(logical_unit w, logical_unit h) override {
            auto* cur = current_widget();
            if (!cur) return {logical_unit(0), logical_unit(0)};
            return cur->measure(w, h);
        }

        void do_arrange(const logical_rect& final_bounds) override {
            auto* cur = current_widget();
            if (!cur) return;
            cur->arrange(this->get_content_area());
        }

    private:
        int m_current_index = -1;

        void update_visibility() {
            auto& ch = this->children();
            for (std::size_t i = 0; i < ch.size(); ++i) {
                ch[i]->set_visible(static_cast<int>(i) == m_current_index);
            }
        }
    };

} // namespace onyxui
