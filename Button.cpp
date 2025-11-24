#include "Button.hpp"

#include "Load.hpp"
#include "Mode.hpp"
#include "PlayMode.hpp"
#include "MenuMode.hpp"
#include "Sound.hpp"


#include <cassert>

// Define static member variables (use default constructor, actual initialization happens in createButtons)
Button Button::MainMenu;
Button Button::Play;
Button Button::QuitGame;

void backToMainMenu()
{
    Sound::stop_all_samples();
    Mode::set_current(std::make_shared<MenuMode>(MenuMode::MenuType::StartMenu));
}

void quitGame()
{
    Sound::stop_all_samples();
    Mode::set_current(nullptr);
}

void playGame()
{
    Sound::stop_all_samples();
    Mode::set_current(std::make_shared<PlayMode>());
}

Load<void> createButtons(LoadTagDefault, []() -> void
                         {
    UIElement resumeButton;
    resumeButton.load_image_data(data_path("resume_button.png"), OriginLocation::UpperLeftOrigin);

    UIElement quitButton;
    quitButton.load_image_data(data_path("quit_button.png"), OriginLocation::UpperLeftOrigin);

    UIElement startButton;
    startButton.load_image_data(data_path("start_button.png"), OriginLocation::UpperLeftOrigin);

    UIElement mainMenuButton;
    mainMenuButton.load_image_data(data_path("back_to_menu_button.png"), OriginLocation::UpperLeftOrigin);

    Button::Play = Button(&playGame, startButton, glm::vec2(0.0f, 0.5f), 0.4f);
    Button::QuitGame = Button(&quitGame, quitButton, glm::vec2(0.0f, 0.0f), 0.2f);
    Button::MainMenu = Button(&backToMainMenu, mainMenuButton, glm::vec2(0.0f, 0.4f), 0.2f); 
});

Button::Button(void (*_callback)(void), UIElement _button, glm::vec2 _position, float _height)
{
    callback = _callback;
    button = _button;
    position = _position;

    height = _height;
    width = _height * float(button.data_width) / float(button.data_height);
    std::cout << "Button created at position (" << position.x << ", " << position.y << ") with size (" << width << ", " << height << ")" << std::endl;
};

Button::~Button()
{
}

bool Button::handle_click(SDL_Event const &evt, glm::uvec2 const &window_size, glm::uvec2 const &drawable_size)
{
    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (evt.button.button == SDL_BUTTON_LEFT)
        {
            glm::vec2 mouse_win(float(evt.button.x), float(evt.button.y));
            glm::vec2 scale = glm::vec2(drawable_size) / glm::vec2(window_size);
            glm::vec2 mouse_px = mouse_win * scale;

            // Get the position of the button in screen space (0 to 1, starting from top-left)
            glm::vec2 button_screen_pos = (1.0f - ((position + 1.0f) / 2.0f)) * glm::vec2(drawable_size);

            std::cout << "Mouse position: " << mouse_px.x << ", " << mouse_px.y << std::endl;
            std::cout << "Button position: " << button_screen_pos.x << ", " << button_screen_pos.y << std::endl;

            width = ((float)window_size.y / window_size.x) * height * (button.data_width / button.data_height);

            // Divide the size by 4 to get half-width and half-height in screen space (clip space is -1 to 1, so we need to account for that)
            if (mouse_px.x >= button_screen_pos.x - (width / 4.0f) * drawable_size.x && mouse_px.x <= button_screen_pos.x + (width / 4.0f) * drawable_size.x &&
                mouse_px.y >= button_screen_pos.y - (height / 4.0f) * drawable_size.y && mouse_px.y <= button_screen_pos.y + (height / 4.0f) * drawable_size.y)
            {
                std::cout << "Button clicked!" << std::endl;
                callback();
                return true;
            }
        }
    }

    return false;
}

void Button::update(float elapsed)
{
}

void Button::draw(glm::uvec2 const &drawable_size)
{
    if (button.data_created)
    {
        button.create_mesh(Mode::window, position.x, position.y, height);
        button.draw_mesh();
    }
}