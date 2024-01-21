#include <array>
#include <optional>
#include <span>

namespace menu {

struct Item {
    enum Action {
        Command,
        Submenu,
    };

    std::string_view title {};
    Action action { Command };
    std::span<Item const* const> submenu {};
    uint32_t id { 0 };
};

template<typename Output, size_t Depth = 10>
class State {
public:
    const Item& rootMenu;
    const Output& output;
    uint32_t menuId { 0 };

    State(const Item& r, const Output& o)
        : rootMenu(r)
        , output(o)
    {
        menuStack[0] = &r;
    }

    auto display() -> void
    {
        if (!displayNeeded) {
            return;
        }
        auto menu = menuStack[current];
        output.write(std::span { "\n\x1b[4;1m" });
        output.write(menu->title);
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

    auto select(char c) -> std::optional<Item const*>
    {
        auto menu = menuStack[current];
        unsigned int selection = c - 48;

        displayNeeded = true;
        if (selection == 0) {
            back();
            return {};
        }

        if (selection >= 1 && selection <= menu->submenu.size()) {
            auto selected = menu->submenu[selection - 1];
            switch (selected->action) {
            case Item::Command:
                return selected;
            case Item::Submenu:
                if (current < menuStack.size()) {
                    ++current;
                    menuStack[current] = selected;
                }
                return {};
            }
        }
        return {};
    }

    auto handle(char c) -> std::optional<Item const*>
    {
        if (menuStack[current]->action == Item::Submenu) {
            output.send(c);
            output.send(10);
            return select(c);
        }
        return {};
    }

protected:
    std::array<Item const*, Depth> menuStack {};
    size_t current { 0 };
    bool displayNeeded { true };
};

} // namespace menu
