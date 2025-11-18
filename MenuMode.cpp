#include "MenuMode.hpp"
#include "Mode.hpp"

#include "Button.hpp"

MenuMode::MenuMode(MenuType menu)
{
}

MenuMode::~MenuMode()
{
}

bool MenuMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
    for (Button b : buttons)
    {
        if (b.handle_click(evt, window_size)) return true;
    }

    return false;
}

void MenuMode::update(float elapsed)
{
}

void MenuMode::draw(glm::uvec2 const &drawable_size)
{
    for (Button b : buttons)
    {
        b.draw(drawable_size);
    }

    if (background.data_created)
    {
        background.draw_mesh();
    }
}