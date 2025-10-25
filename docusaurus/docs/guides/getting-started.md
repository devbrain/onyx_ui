---
sidebar_position: 1
---

# Getting Started

This guide will walk you through the process of setting up your first OnyxUI application. We'll cover the prerequisites, how to build the library, and how to create a simple "Hello World" application.

## Prerequisites

Before you begin, you'll need a few things:

- **A C++20 compatible compiler:** OnyxUI uses modern C++20 features, so you'll need a recent compiler. The following are known to work:
    - GCC 10 or later
    - Clang 12 or later
    - MSVC 2019 or later
- **CMake:** You'll need CMake version 3.15 or later to build the library.
- **A backend:** OnyxUI is backend-agnostic, but to get started, you'll need a backend to render your UI. The library comes with a `conio` backend for creating terminal-based UIs, which is a great way to get started without any additional dependencies.

## Building the Library

OnyxUI is a header-only library, so there's no need to build it separately. However, the repository includes a number of examples and tests that you can build to verify your setup.

Here's how to build the included examples and tests:

1.  **Clone the repository:**

    ```bash
    git clone https://github.com/your-username/onyxui.git
    cd onyxui
    ```

2.  **Configure with CMake:**

    ```bash
    cmake -B build
    ```

3.  **Build the project:**

    ```bash
    cmake --build build -j8
    ```

4.  **Run the tests:**

    ```bash
    ./build/bin/ui_unittest
    ```

    You should see a message indicating that all tests have passed.

## Your First Application: Hello World

Now that you have your environment set up, let's create a simple "Hello World" application. We'll use the `conio` backend to create a terminal-based UI.

1.  **Create a new C++ file:**

    Create a new file named `main.cc` and add the following code:

    ```cpp
    #include <onyxui/conio/conio_backend.hh>
    #include <onyxui/ui_handle.hh>
    #include <onyxui/ui_context.hh>
    #include <onyxui/widgets/vbox.hh>
    #include <onyxui/widgets/button.hh>
    #include <onyxui/widgets/label.hh>
    #include <iostream>

    int main() {
        using Backend = onyxui::conio::conio_backend;

        // 1. Create UI context (provides services: layers, focus, themes, background)
        onyxui::scoped_ui_context<Backend> ctx;

        // 2. Configure background (optional - defaults to black solid)
        auto* bg = onyxui::ui_services<Backend>::background();
        if (bg) {
            bg->set_mode(onyxui::background_mode::solid);
            bg->set_color({0, 0, 170});  // Blue background
        }

        // 3. Build UI widget tree
        auto root = onyxui::create_vbox<Backend>();

        auto title = onyxui::create_label<Backend>("Welcome to OnyxUI!");
        root->add_child(std::move(title));

        auto button = onyxui::create_button<Backend>("Click Me");
        button->clicked().connect([]() {
            std::cout << "Button clicked!" << std::endl;
        });
        root->add_child(std::move(button));

        // 4. Create UI handle (manages rendering pipeline)
        onyxui::ui_handle<Backend> ui(std::move(root));

        // 5. Main event loop
        bool quit = false;
        while (!quit) {
            // Render frame: background -> widgets -> layers
            ui.display();
            ui.present();

            // Handle events (simplified - real code would poll events)
            // if (user_pressed_escape()) quit = true;
            break;  // For this simple example, just render once
        }

        return 0;
    }
    ```

2.  **Compile and run:**

    You can compile this file with your C++20 compiler, making sure to include the `onyxui/include` directory.

    ```bash
    g++ -std=c++20 -I./include main.cc -o hello_onyxui
    ./hello_onyxui
    ```

    While this simple example doesn't have an interactive event loop, it demonstrates the basic principles of creating a UI with OnyxUI:

    -   **Create a root element:** In this case, a `vbox`.
    -   **Add child widgets:** We add a `label` and a `button`.
    -   **Connect event handlers:** We connect a lambda to the button's `clicked` signal.
    -   **Measure and arrange:** We call `measure()` and `arrange()` to perform the layout calculations.

## Next Steps

Congratulations, you've created your first OnyxUI application! From here, you can explore the other guides in this documentation to learn about more advanced topics:

-   **[Core Concepts](../core-concepts/backend-pattern.md):** Learn about the fundamental design patterns that underpin the library.
-   **[API Reference](../api-reference/ui-element.md):** Dive deep into the details of the `ui_element` class and other key components.
