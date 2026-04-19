#include <memory>
#include <onyxui/for/sdlpp.hh>

int main() {
    using namespace onyxui::simple;
    app_window win("Hello", 640, 480);
    auto root = std::make_unique<vbox>();
    root->emplace_child<::onyxui::label>("Hello, World!");
    auto* ok = root->emplace_child<::onyxui::button>("OK");
    ok->clicked.connect([&] { win.close(); });
    win.set_content(std::move(root));
    win.show();
    return run();
}
