#include <array>
#include <span>
#include <string>
#include <variant>

#include "accept.h"

namespace menu {

struct Item {
    enum Action {
        Command,
        Input,
        Submenu,
    };

    std::string_view title {};
    Action action { Command };
    std::span<Item const* const> submenu {};
    uint32_t id { 0 };
};

consteval auto depth(Item const* const menu) -> size_t
{
    if (menu->action != Item::Submenu) {
        return 1;
    }
    size_t d = 0;
    for (auto m : menu->submenu) {
        d = std::max(d, depth(m));
    }
    return d + 1;
}

consteval auto maxWidth(Item const* const menu, size_t n) -> size_t
{
    n = std::max(n, menu->title.size());
    for (auto m : menu->submenu) {
        n = std::max(n, maxWidth(m, n));
    }
    return n;
}

struct Select {};

struct Command {
    uint32_t command;
};

struct Input {
    std::span<uint8_t> input;
    uint32_t id;
};

typedef std::variant<Select, Command, Input> Result;
template<class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

template<Item const* const rootMenu, size_t inputSize, typename Output>
class State {
public:
    uint32_t menuId;
    const Output& output;
    accept::Accept<inputSize, Output> accept { output };

    State(const Output& o)
        : output(o)
    {
        menuStack[0] = rootMenu;
    }

    auto display() -> void
    {
        if (!displayNeeded) {
            return;
        }
        auto menu = menuStack[current];
        output.write(std::span { "\n\x1b[4;1m" });
        output.write(menu->title);
        for (size_t n = 0; n < maxWidth(rootMenu, 0) + 3 - menu->title.size(); n++) {
            output.send(' ');
        }
        output.write(std::span { "\x1b[0m\n\n" });
        for (uint8_t i = 1; auto m : menu->submenu) {
            output.send(static_cast<uint8_t>(i + '0'));
            output.write(std::span { ") " });
            output.write(m->title);
            if (m->action == Item::Submenu) {
                output.write(std::span { "..." });
            }
            output.send('\n');
            ++i;
        }
        output.write(std::span { "\n> " });
        displayNeeded = false;
    }

    auto back() -> void
    {
        if (current > 0) {
            --current;
            displayNeeded = true;
        }
    }

    auto select(char c) -> Result
    {
        auto menu = menuStack[current];
        unsigned int selection = c - 48;

        displayNeeded = true;
        if (selection == 0) {
            back();
            return Result { Select {} };
        }

        if (selection >= 1 && selection <= menu->submenu.size()) {
            auto selected = menu->submenu[selection - 1];
            menuId = selected->id;
            if (selected->action == Item::Command) {
                return Result { Command { menuId } };
            } else {
                if (current < depth(rootMenu) - 1) {
                    ++current;
                    menuStack[current] = selected;
                }
                if (selected->action == Item::Input) {
                    accept.reset();
                }
            }
            return Result { Select {} };
        }
        return Result { Select {} };
    }

    auto input(uint8_t c) -> Result
    {
        if (accept.handle(c) == accept::Accepted) {
            back();
            return Result { Input { accept.accepted(), menuId } };
        }
        return Result { Select {} };
    }

    auto handle(char c) -> Result
    {
        switch (menuStack[current]->action) {
        case Item::Submenu:
            output.send(c);
            output.send(10);
            return select(c);
        case Item::Input:
            return input(c);
        default:
            break;
        }
        return Result { Select {} };
    }

protected:
    std::array<Item const*, depth(rootMenu)> menuStack {};
    size_t current { 0 };
    bool displayNeeded { true };
};

} // namespace menu
