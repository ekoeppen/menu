#include <array>
#include <span>

#include "board.h"
#include "menu.h"

constexpr menu::Item mode{.title = "Mode", .action = menu::Item::Input};
constexpr menu::Item txInterval{.title = "Transmit interval", .action = menu::Item::Input};
constexpr menu::Item txData{.title = "Transmit data", .action = menu::Item::Input};
constexpr auto settingsItems = std::array{&mode, &txInterval, &txData};
constexpr menu::Item settings{
    .title = "Settings",
    .action = menu::Item::Submenu,
    .submenu = settingsItems,
};

constexpr menu::Item txTest{.title = "Transmit test", .id = 1};
constexpr menu::Item rxTest{.title = "Receive test", .id = 2};
constexpr auto toplevelItems = std::array{&txTest, &rxTest, &settings};
constexpr menu::Item toplevel{
    .title = "Menu",
    .action = menu::Item::Submenu,
    .submenu = toplevelItems,
};

auto serial_write(uint8_t c) -> void { board::serial.send(c); }

auto main() -> int {
  board::init();
  board::led.init();
  board::led.set();
  board::tx.init();
  board::rx.init();
  board::serial.init();

  auto menu = menu::State<&toplevel, serial_write, 40>();
  menu.display();
  using State = decltype(menu);
  while (true) {
    menu.display();
    switch (menu.handle(board::serial.receive())) {
    case State::Select:
      break;
    case State::Command:
      board::serial.write(std::span{"Command "});
      board::serial.send(static_cast<uint8_t>(menu.menuId + '0'));
      board::serial.send(10);
      break;
    case State::Input:
      board::serial.write(std::span{"\nInput: "});
      board::serial.write(menu.accept.accepted());
      break;
    }
  }

  return 0;
}
