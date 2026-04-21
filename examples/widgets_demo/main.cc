// OnyxUI Widgets Demo — simple-shell entry point.
#include "widgets_demo.hh"

int main() {
    ui::app_window win("OnyxUI Widgets Demo", 1024, 768);
    win.set_content(build_widgets_demo(win));
    win.show();
    return ui::run();
}
