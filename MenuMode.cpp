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
        break;

    case MenuType::PauseMenu:
        buttons.push_back(Button::Resume);
        buttons.push_back(Button::MainMenu);
        background.load_image_data(data_path("TitleSlide3.png"), OriginLocation::UpperLeftOrigin);
        break;

    case MenuType::StartMenu:
        buttons.push_back(Button::Play);
        buttons.push_back(Button::QuitGame);
        background.load_image_data(data_path("TitleSlide2.png"), OriginLocation::UpperLeftOrigin);
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
    for (Button &b : buttons)
    {
        b.update(elapsed);
    }
}

void MenuMode::draw(glm::uvec2 const &drawable_size)
{
    glUseProgram(lit_color_texture_program->program);
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
    glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
    glUseProgram(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

    if (background.data_created)
    {
        background.create_mesh(Mode::window, 0.0f, 0.0f, 2.0f);
        background.draw_mesh();
    }

    for (Button b : buttons)
    {
        b.draw(drawable_size);
    }
}