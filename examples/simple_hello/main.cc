#include <memory>

// The only backend-specific line: the bundle header. CMake defines
// ONYXUI_EXAMPLE_USE_CONIO=1 for the conio-backed target; the default
// is sdlpp. Everything below this point is identical across backends
// — that's the whole point of the simple shell.
#if defined(ONYXUI_EXAMPLE_USE_CONIO)
#  include <onyxui/for/conio.hh>
#else
#  include <onyxui/for/sdlpp.hh>
#endif

int main() {
    using namespace onyxui::simple;
    app_window win("Hello", 640, 480);
    auto root = std::make_unique<vbox>();
    root->emplace_child<label>("Hello, World!");
    auto* ok = root->emplace_child<button>("OK");
    ok->clicked.connect([&] { win.close(); });
    win.set_content(std::move(root));
    win.show();
    return run();
}
