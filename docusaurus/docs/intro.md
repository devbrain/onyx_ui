---
sidebar_position: 1
---

# Introduction to OnyxUI

Welcome to the official documentation for OnyxUI, a modern C++20 UI framework designed for performance, flexibility, and ease of use. This documentation is your comprehensive guide to understanding, using, and extending the library.

## What is OnyxUI?

OnyxUI is a lightweight, header-only UI framework that provides a powerful set of tools for building sophisticated user interfaces in C++. Its core philosophy is to offer a robust and extensible foundation that is not tied to any specific rendering backend or platform.

## Core Features

- **Two-Pass Layout System**: An efficient measure/arrange algorithm with smart caching ensures high-performance layout calculations, even for complex UI hierarchies.
- **Backend Agnostic**: OnyxUI is designed to work with any rendering engine. Through the power of C++20 concepts, you can integrate it with your own renderer, whether it's based on SDL2, Conio, or a custom solution.
- **Rich Widget Library**: A comprehensive set of widgets is included out of the box, such as buttons, labels, menus, panels, and grids, allowing you to build complex UIs quickly.
- **CSS-Style Theming**: The theming system allows for easy customization of the look and feel of your application. Properties are inherited from parent to child, similar to CSS, making it easy to create consistent and beautiful designs.
- **Thread-Safe Signals**: The signal/slot system provides a powerful mechanism for event handling and is thread-safe by default, making it suitable for use in multi-threaded applications.
- **Modern C++ Design**: The library is written in modern C++20 and leverages features like concepts, move semantics, and smart pointers to provide a safe and expressive API.

## Who is this documentation for?

This documentation is intended for a wide audience, from developers who are new to OnyxUI to experienced contributors who want to dive deep into the library's internals.

- **For Newcomers:** The "Guides" section provides a series of tutorials that will walk you through the basics of creating your first OnyxUI application.
- **For Experienced Developers:** The "Core Concepts" and "API Reference" sections provide a detailed breakdown of the library's architecture and a comprehensive reference to all its components.

## Getting Started

If you're ready to start building with OnyxUI, we recommend heading over to the **[Getting Started](./guides/getting-started.md)** guide. It will walk you through the process of setting up your development environment and creating your first "Hello World" application.
