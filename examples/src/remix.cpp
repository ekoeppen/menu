#include <array>
#include <span>
#include <string>

#include "accept.h"
#include "menu.h"
#include "remix.h"

using namespace remix;

enum Commands {
    Mode,
    TransmitInterval,
    TransmitData,
    TransmitTest,
    ReceiveTest,
};

constexpr menu::Item mode { .title = "Mode", .id = Mode };
constexpr menu::Item txInterval { .title = "Transmit interval", .id = TransmitInterval };
constexpr menu::Item txData { .title = "Transmit data", .id = TransmitData };
constexpr auto settingsItems = std::array { &mode, &txInterval, &txData };
constexpr menu::Item settings {
    .title = "Settings",
    .action = menu::Item::Submenu,
    .submenu = settingsItems,
};

constexpr menu::Item txTest { .title = "Transmit test", .id = TransmitTest };
constexpr menu::Item rxTest { .title = "Receive test", .id = ReceiveTest };
constexpr auto toplevelItems = std::array { &txTest, &rxTest, &settings };
constexpr menu::Item toplevel {
    .title = "Menu",
    .action = menu::Item::Submenu,
    .submenu = toplevelItems,
};

auto m = menu::State(toplevel, board::serial);
auto a = accept::Accept(board::serial);

auto handleCommand(menu::Item const* const item) -> void
{
    board::serial.write(std::span { "Command " });
    board::serial.send(static_cast<uint8_t>(item->id + '0'));
    board::serial.send(10);
}

auto handleInput(menu::Item const* const item) -> void
{
    board::serial.write(item->title);
    board::serial.write(std::span { ": " });
    a.reset();
    while (true) {
        auto result = a.handle(board::serial.receive());
        switch (result) {
        case accept::Canceled:
            board::serial.write(std::span { "\nCanceled.\n" });
            return;
        case accept::Accepted:
            board::serial.write(std::span { "\nDone: " });
            board::serial.write(a.accepted());
            board::serial.send('\n');
            return;
        default:
            break;
        }
    }
}

auto main() -> int
{
    board::init();
    board::led.init();
    board::led.set();
    board::tx.init();
    board::rx.init();
    board::serial.init();

    while (true) {
        m.display();
        auto r = m.handle(board::serial.receive());
        if (r) {
            switch (r.value()->id) {
            case Mode:
            case TransmitInterval:
            case TransmitData:
                handleInput(r.value());
                break;
            case TransmitTest:
            case ReceiveTest:
                handleCommand(r.value());
                break;
            }
        }
    }

    return 0;
}
