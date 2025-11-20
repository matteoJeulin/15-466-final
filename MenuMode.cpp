#include "MenuMode.hpp"
#include "Mode.hpp"

#include "Button.hpp"

MenuMode::MenuMode(MenuType menu)
{
    std::cout << "MenuMode created! " << menu << std::endl;
    switch (menu)
    {
    case MenuType::EndMenu:
        buttons.push_back(Button::MainMenu);
        buttons.push_back(Button::QuitGame);
        background.load_image_data(data_path("CheeseWin.png"), OriginLocation::UpperLeftOrigin);
        background.create_mesh(Mode::window, 400.0f, 300.0f, 600.0f);
        break;

    case MenuType::PauseMenu:
        buttons.push_back(Button::Resume);
        buttons.push_back(Button::MainMenu);
        background.load_image_data(data_path("TitleSlide3.png"), OriginLocation::UpperLeftOrigin);
        background.create_mesh(Mode::window, 400.0f, 300.0f, 600.0f);
        break;

    case MenuType::StartMenu:
        buttons.push_back(Button::Play);
        buttons.push_back(Button::QuitGame);
        background.load_image_data(data_path("TitleSlide2.png"), OriginLocation::UpperLeftOrigin);
        background.create_mesh(Mode::window, 400.0f, 300.0f, 600.0f);
        break;

    default:
        std::cerr << "MenuMode::MenuMode(): Unknown menu type!" << std::endl;
        break;
    }
}

MenuMode::~MenuMode()
{
}

bool MenuMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
    for (Button b : buttons)
    {
        if (b.handle_click(evt, window_size))
            return true;
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