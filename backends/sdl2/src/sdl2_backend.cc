//
// SDL2 Backend Implementation
//

#include <onyxui/sdl2/sdl2_backend.hh>
#include "sdl2_themes.hh"

namespace onyxui::sdl2 {

void sdl2_backend::register_themes(onyxui::theme_registry<sdl2_backend>& registry) {
    sdl2_themes::register_themes(registry);
}

} // namespace onyxui::sdl2
