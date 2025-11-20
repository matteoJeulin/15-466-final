#include "Button.hpp"

#include "Load.hpp"

#include <cassert>

// Define static member variables (use default constructor, actual initialization happens in createButtons)
Button Button::MainMenu;
Button Button::Resume;
Button Button::Play;
Button Button::QuitGame;

void backToMainMenu()
{
    Mode::set_current(std::make_shared<MenuMode>(MenuMode::MenuType::StartMenu));
}

void quitGame()
{
    Mode::set_current(nullptr);
}

void playGame()
{
    Mode::set_current(std::make_shared<PlayMode>());
}

Load<void> createButtons(LoadTagDefault, []() -> void {
    UIElement resumeButton = UIElement();
    resumeButton.load_image_data(data_path("resume_button.png"), OriginLocation::UpperLeftOrigin);
    resumeButton.create_mesh(Mode::window, 300.0f + float(resumeButton.data_width) / 2.0f, 200.0f + float(resumeButton.data_height) / 2.0f, float(resumeButton.data_height));

    UIElement quitButton = UIElement();
    quitButton.load_image_data(data_path("quit_button.png"), OriginLocation::UpperLeftOrigin);
    quitButton.create_mesh(Mode::window, 300.0f + float(quitButton.data_width) / 2.0f, 100.0f + float(quitButton.data_height) / 2.0f, float(quitButton.data_height));

    UIElement startButton = UIElement();
    startButton.load_image_data(data_path("start_button.png"), OriginLocation::UpperLeftOrigin);
    startButton.create_mesh(Mode::window, 300.0f + float(startButton.data_width) / 2.0f, 300.0f + float(startButton.data_height) / 2.0f, float(startButton.data_height));

    UIElement mainMenuButton = UIElement();
    mainMenuButton.load_image_data(data_path("start_button.png"), OriginLocation::UpperLeftOrigin); // TODO: change to proper asset
    mainMenuButton.create_mesh(Mode::window, 300.0f + float(mainMenuButton.data_width) / 2.0f, 400.0f + float(mainMenuButton.data_height) / 2.0f, float(mainMenuButton.data_height));

    Button::Resume = Button(&backToMainMenu, resumeButton, glm::vec2(300.0f, 200.0f));
    Button::QuitGame = Button(&quitGame, quitButton, glm::vec2(300.0f, 100.0f));
    Button::Play = Button(&playGame, startButton, glm::vec2(300.0f, 300.0f));
    Button::MainMenu = Button(&backToMainMenu, mainMenuButton, glm::vec2(300.0f, 400.0f));
});

Button::Button(void (*_callback)(void), UIElement _button, glm::vec2 _position)
{
    callback = _callback;
    button = _button;
    position = _position;

    width = float(button.data_width);
    height = float(button.data_height);

    button.create_mesh(Mode::window, position.x + width / 2.0f, position.y + height / 2.0f, height);
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
    if (button.data_created)
    {
        button.draw_mesh();
    }
}