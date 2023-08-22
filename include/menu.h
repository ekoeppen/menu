#include <array>
#include <span>
#include <string>

#include "accept.h"

namespace menu {

struct Item {
  enum Action {
    Command,
    Input,
    Submenu,
  };

  std::string_view title{};
  Action action{Command};
  std::span<Item const *const> submenu{};
  uint32_t id{0};
};

consteval auto depth(Item const *const menu) -> size_t {
  if (menu->action != Item::Submenu) {
    return 1;
  }
  size_t d = 0;
  for (auto m : menu->submenu) {
    d = std::max(d, depth(m));
  }
  return d + 1;
}

consteval auto maxWidth(Item const *const menu, size_t n) -> size_t {
  n = std::max(n, menu->title.size());
  for (auto m : menu->submenu) {
    n = std::max(n, maxWidth(m, n));
  }
  return n;
}

template <Item const *const rootMenu, auto write, size_t inputSize> class State {
public:
  enum Result { Select, Command, Input };

  uint32_t menuId;
  accept::Accept<inputSize, write> accept;

  State() { menuStack[0] = rootMenu; }

  template <typename S> auto print(S const s) {
    for (auto c : s) {
      write(c);
    }
  }

  auto display() -> void {
    if (!displayNeeded) {
      return;
    }
    auto menu = menuStack[current];
    print(std::span{"\n\x1b[4;1m"});
    print(std::span{menu->title});
    for (size_t n = 0; n < maxWidth(rootMenu, 0) + 3 - menu->title.size(); n++) {
      write(' ');
    }
    print(std::span{"\x1b[0m\n\n"});
    for (uint8_t i = 1; auto m : menu->submenu) {
      write(static_cast<uint8_t>(i + '0'));
      print(std::span{") "});
      print(std::span{m->title});
      write('\n');
      ++i;
    }
    displayNeeded = false;
  }

  auto back() -> void {
    if (current > 0) {
      --current;
      displayNeeded = true;
    }
  }

  auto select(char c) -> Result {
    Result r = Select;
    auto menu = menuStack[current];
    unsigned int selection = c - 48;
    if (selection >= 1 && selection <= menu->submenu.size()) {
      auto selected = menu->submenu[selection - 1];
      if (selected->action == Item::Command) {
        menuId = selected->id;
        r = Command;
      } else {
        if (current < depth(rootMenu) - 1) {
          ++current;
          menuStack[current] = selected;
        }
        if (selected->action == Item::Input) {
          accept.reset();
        }
      }
    } else {
      if (selection == 0) {
        back();
      }
    }
    displayNeeded = true;
    return r;
  }

  auto input(uint8_t c) -> Result {
    switch (accept.handle(c)) {
    case accept::Accepted:
      back();
      return Input;
    default:
      break;
    }
    return Select;
  }

  auto handle(char c) -> Result {
    switch (menuStack[current]->action) {
    case Item::Submenu:
      return select(c);
    case Item::Input:
      return input(c);
    default:
      break;
    }
    return Select;
  }

protected:
  std::array<Item const *, depth(rootMenu)> menuStack{};
  size_t current{0};
  bool displayNeeded{true};
};

} // namespace menu
