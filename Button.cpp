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

Load<void> createButtons(LoadTagDefault, []() -> void
                         {
    UIElement resumeButton;
    resumeButton.load_image_data(data_path("resume_button.png"), OriginLocation::UpperLeftOrigin);
    resumeButton.create_mesh(Mode::window, 1.0f, 0.7f, 1.0f);

    UIElement quitButton;
    quitButton.load_image_data(data_path("quit_button.png"), OriginLocation::UpperLeftOrigin);
    quitButton.create_mesh(Mode::window, 1.0f, 0.6f, 1.0f);

    UIElement startButton;
    startButton.load_image_data(data_path("start_button.png"), OriginLocation::UpperLeftOrigin);
    startButton.create_mesh(Mode::window, 1.0f, 0.5f, 1.0f);

    UIElement mainMenuButton;
    mainMenuButton.load_image_data(data_path("back_to_menu_button.png"), OriginLocation::UpperLeftOrigin);
    mainMenuButton.create_mesh(Mode::window, 1.0f, 0.4f, 1.0f);

    Button::Resume = Button(&backToMainMenu, resumeButton, glm::vec2(1.0f, 0.7f));
    Button::QuitGame = Button(&quitGame, quitButton, glm::vec2(1.0f, 0.6f));
    Button::Play = Button(&playGame, startButton, glm::vec2(1.0f, 0.5f));
    Button::MainMenu = Button(&backToMainMenu, mainMenuButton, glm::vec2(1.0f, 0.4f)); });

Button::Button(void (*_callback)(void), UIElement _button, glm::vec2 _position)
{
    callback = _callback;
    button = _button;
    position = _position;

    width = float(button.data_width);
    height = float(button.data_height);

    button.create_mesh(Mode::window, position.x, position.y, 1.0f);
};

bool Button::handle_click(SDL_Event const &evt, glm::uvec2 const &window_size)
{
    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (evt.button.button == SDL_BUTTON_LEFT)
        {
            std::cout << "Cool" << std::endl;
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

void Button::update(float elapsed)
{
    button.create_mesh(Mode::window, position.x, position.y, 1.0f);
}

void Button::draw(glm::uvec2 const &drawable_size)
{
    last_drawable_px = drawable_size;

    if (button.data_created)
    {
        button.draw_mesh();
    }
}