//
// Simple termbox2 event tester
// Prints all events received from termbox2
//

#define TB_IMPL
#include <termbox2.h>
#include <iostream>
#include <cstdio>

const char* event_type_name(int type) {
    switch(type) {
        case TB_EVENT_KEY: return "TB_EVENT_KEY";
        case TB_EVENT_RESIZE: return "TB_EVENT_RESIZE";
        case TB_EVENT_MOUSE: return "TB_EVENT_MOUSE";
        default: return "UNKNOWN";
    }
}

const char* key_name(uint16_t key) {
    switch(key) {
        case TB_KEY_ESC: return "ESC";
        case TB_KEY_ENTER: return "ENTER";
        case TB_KEY_TAB: return "TAB";
        case TB_KEY_SPACE: return "SPACE";
        case TB_KEY_F1: return "F1";
        case TB_KEY_F2: return "F2";
        case TB_KEY_F3: return "F3";
        case TB_KEY_F4: return "F4";
        case TB_KEY_F5: return "F5";
        case TB_KEY_F6: return "F6";
        case TB_KEY_F7: return "F7";
        case TB_KEY_F8: return "F8";
        case TB_KEY_F9: return "F9";
        case TB_KEY_F10: return "F10";
        case TB_KEY_F11: return "F11";
        case TB_KEY_F12: return "F12";
        default: return nullptr;
    }
}

int main() {
    // Initialize termbox2
    if (tb_init() != TB_OK) {
        std::cerr << "Failed to initialize termbox2" << std::endl;
        return 1;
    }

    // Enable mouse input
    tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

    // Clear screen and show instructions
    tb_clear();
    const char* msg = "Termbox2 Event Monitor - Press ESC to quit, try resizing terminal";
    for (size_t i = 0; msg[i]; ++i) {
        tb_set_cell(i, 0, msg[i], TB_WHITE, TB_BLACK);
    }
    tb_present();

    std::cout << "\n=== Termbox2 Event Monitor ===" << std::endl;
    std::cout << "Resize the terminal window and watch for events" << std::endl;
    std::cout << "Press ESC to quit\n" << std::endl;

    int event_count = 0;

    // Event loop
    while (true) {
        tb_event ev;
        int result = tb_poll_event(&ev);

        if (result != TB_OK) {
            std::cerr << "\n*** tb_poll_event returned error code: " << result;

            // Translate error codes
            switch(result) {
                case TB_ERR_INIT_ALREADY: std::cerr << " (TB_ERR_INIT_ALREADY)"; break;
                case TB_ERR_INIT_OPEN: std::cerr << " (TB_ERR_INIT_OPEN)"; break;
                case TB_ERR_MEM: std::cerr << " (TB_ERR_MEM)"; break;
                case TB_ERR_NO_EVENT: std::cerr << " (TB_ERR_NO_EVENT)"; break;
                case TB_ERR_NO_TERM: std::cerr << " (TB_ERR_NO_TERM)"; break;
                case TB_ERR_NOT_INIT: std::cerr << " (TB_ERR_NOT_INIT)"; break;
                case TB_ERR_OUT_OF_BOUNDS: std::cerr << " (TB_ERR_OUT_OF_BOUNDS)"; break;
                case TB_ERR_UNSUPPORTED_TERM: std::cerr << " (TB_ERR_UNSUPPORTED_TERM)"; break;
                case TB_ERR_RESIZE_IOCTL: std::cerr << " (TB_ERR_RESIZE_IOCTL)"; break;
                case TB_ERR_READ: std::cerr << " (TB_ERR_READ)"; break;
                case TB_ERR_RESIZE_PIPE: std::cerr << " (TB_ERR_RESIZE_PIPE)"; break;
                case TB_ERR_RESIZE_SIGACTION: std::cerr << " (TB_ERR_RESIZE_SIGACTION)"; break;
                case TB_ERR_POLL: std::cerr << " (TB_ERR_POLL)"; break;
                case TB_ERR_TCGETATTR: std::cerr << " (TB_ERR_TCGETATTR)"; break;
                case TB_ERR_TCSETATTR: std::cerr << " (TB_ERR_TCSETATTR)"; break;
                case TB_ERR_RESIZE_WRITE: std::cerr << " (TB_ERR_RESIZE_WRITE)"; break;
                case TB_ERR_RESIZE_POLL: std::cerr << " (TB_ERR_RESIZE_POLL)"; break;
                case TB_ERR_RESIZE_READ: std::cerr << " (TB_ERR_RESIZE_READ)"; break;
                case TB_ERR_RESIZE_SSCANF: std::cerr << " (TB_ERR_RESIZE_SSCANF)"; break;
                default: std::cerr << " (UNKNOWN)"; break;
            }
            std::cerr << " ***" << std::endl;
            std::cerr << "Continuing anyway...\n" << std::endl;
            continue;  // Don't break, keep going
        }

        event_count++;

        std::cout << "Event #" << event_count << ": "
                  << "type=" << event_type_name(ev.type)
                  << " (" << (int)ev.type << ")";

        switch (ev.type) {
            case TB_EVENT_KEY:
                std::cout << ", key=" << ev.key;
                if (const char* kn = key_name(ev.key)) {
                    std::cout << " (" << kn << ")";
                }
                std::cout << ", ch=" << ev.ch;
                if (ev.ch >= 32 && ev.ch < 127) {
                    std::cout << " ('" << (char)ev.ch << "')";
                }
                std::cout << ", mod=" << (int)ev.mod;
                break;

            case TB_EVENT_RESIZE:
                std::cout << ", width=" << tb_width()
                          << ", height=" << tb_height()
                          << " [w=" << ev.w << ", h=" << ev.h << "]";
                break;

            case TB_EVENT_MOUSE:
                std::cout << ", x=" << ev.x
                          << ", y=" << ev.y
                          << ", key=" << ev.key;
                break;
        }

        std::cout << std::endl;

        // Exit on ESC
        if (ev.type == TB_EVENT_KEY && ev.key == TB_KEY_ESC) {
            std::cout << "\nESC pressed - exiting" << std::endl;
            break;
        }
    }

    tb_shutdown();
    return 0;
}
