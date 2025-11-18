#include "Button.hpp"

#include <cassert>

Button::Button(void (*_callback)(void), UIElement _button)
{
    callback = _callback;
    button = _button;
};

bool Button::handle_click(SDL_Event const &evt, glm::uvec2 const &window_size)
{
    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (evt.button.button == SDL_BUTTON_LEFT)
        {
            glm::uvec2 last_drawable_px = glm::uvec2(1, 1);

            glm::vec2 mouse_win(float(evt.button.x), float(evt.button.y));
            glm::vec2 scale = glm::vec2(last_drawable_px) / glm::vec2(window_size);
            glm::vec2 mouse_px = mouse_win * scale;

            if (mouse_px.x >= position.x && mouse_px.x <= position.x + width &&
                mouse_px.y >= position.y && mouse_px.y <= position.y + height)
            {
                callback();
                return true;
            }
        }
    }

    return false;
}

void Button::draw(glm::uvec2 const &drawable_size)
{
	assert(button.data_created);
    if (button.data_created)
    {
        button.draw_mesh();
    }
}

void backToMainMenu() {
    Mode::set_current(std::make_shared<MenuMode>(MenuMode::MenuType::StartMenu));
}