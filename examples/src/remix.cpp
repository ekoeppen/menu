#include <array>
#include <span>

#include "board.h"
#include "menu.h"

using namespace remix;

constexpr menu::Item mode { .title = "Mode", .action = menu::Item::Input };
constexpr menu::Item txInterval { .title = "Transmit interval", .action = menu::Item::Input };
constexpr menu::Item txData { .title = "Transmit data", .action = menu::Item::Input };
constexpr auto settingsItems = std::array { &mode, &txInterval, &txData };
constexpr menu::Item settings {
    .title = "Settings",
    .action = menu::Item::Submenu,
    .submenu = settingsItems,
};

constexpr menu::Item txTest { .title = "Transmit test", .id = 1 };
constexpr menu::Item rxTest { .title = "Receive test", .id = 2 };
constexpr auto toplevelItems = std::array { &txTest, &rxTest, &settings };
constexpr menu::Item toplevel {
    .title = "Menu",
    .action = menu::Item::Submenu,
    .submenu = toplevelItems,
};

auto main() -> int
{
    board::init();
    board::led.init();
    board::led.set();
    board::tx.init();
    board::rx.init();
    board::serial.init();

    auto menu = menu::State<&toplevel, 40, decltype(board::serial)>(board::serial);
    while (true) {
        menu.display();
        auto r = menu.handle(board::serial.receive());
        std::visit(menu::overload {
                       [](menu::Command cmd) {
                           board::serial.write(std::span { "Command " });
                           board::serial.send(static_cast<uint8_t>(cmd.command + '0'));
                           board::serial.send(10);
                       },
                       [](menu::Input input) {
                           board::serial.write(std::span { "\nInput: " });
                           board::serial.write(input.input);
                       },
                       []([[maybe_unused]] menu::Select select) {},
                   },
            r);
    }

    return 0;
}
