// OnyxUI Widgets Demo — simple-shell entry point.
#include "widgets_demo.hh"

int main() {
    onyxui::simple::app_window win("OnyxUI Widgets Demo", 1024, 768);
    win.set_content(build_widgets_demo(win));
    win.show();
    return onyxui::simple::run();
}
