#include "MenuMode.hpp"
#include "Mode.hpp"

#include "Button.hpp"

std::unordered_map<MenuMode::Rank, UIElement> MenuMode::rankSprites;

Load<void> loadScores(LoadTagDefault, []() -> void
                      {
    MenuMode::rankSprites = std::unordered_map<MenuMode::Rank, UIElement>();

    UIElement SScore;
    SScore.load_image_data(data_path("S_tier.png"), OriginLocation::UpperLeftOrigin);
    MenuMode::rankSprites.emplace(MenuMode::S, SScore);

    UIElement AScore;
    AScore.load_image_data(data_path("A_tier.png"), OriginLocation::UpperLeftOrigin);
    MenuMode::rankSprites.emplace(MenuMode::A, AScore);

    UIElement BScore;
    BScore.load_image_data(data_path("B_tier.png"), OriginLocation::UpperLeftOrigin);
    MenuMode::rankSprites.emplace(MenuMode::B, BScore);

    UIElement CScore;
    CScore.load_image_data(data_path("C_tier.png"), OriginLocation::UpperLeftOrigin);
    MenuMode::rankSprites.emplace(MenuMode::C, CScore);

    UIElement DScore;
    DScore.load_image_data(data_path("S_tier.png"), OriginLocation::UpperLeftOrigin);
    MenuMode::rankSprites.emplace(MenuMode::D, DScore);

    UIElement FScore;
    FScore.load_image_data(data_path("F_tier.png"), OriginLocation::UpperLeftOrigin);
    MenuMode::rankSprites.emplace(MenuMode::F, FScore);

    UIElement NoScore;
    NoScore.load_image_data(data_path("F_tier.png"), OriginLocation::UpperLeftOrigin); // Use F for no tier
    MenuMode::rankSprites.emplace(MenuMode::NO_RANK, NoScore); });

MenuMode::MenuMode(MenuType menu, Rank _rank, int _score)
{
    rank = _rank;
    score = _score;

    switch (menu)
    {
    case MenuType::LevelClearMenu:
        title = "Level cleared";
        title_x_pos = 150.0f;
        buttons.push_back(Button::NextLevel);
        buttons.push_back(Button::MainMenu);
        buttons.push_back(Button::QuitGame);
        background.load_image_data(data_path("CheeseWin.png"), OriginLocation::UpperLeftOrigin);
        break;

    case MenuType::WinMenu:
        title = "You won";
        title_x_pos = 120.0f;
        buttons.push_back(Button::MainMenu);
        buttons.push_back(Button::QuitGame);
        background.load_image_data(data_path("CheeseWin.png"), OriginLocation::UpperLeftOrigin);
        break;

    case MenuType::LoseMenu:
        title = "Game Over";
        title_x_pos = 120.0f;
        buttons.push_back(Button::MainMenu);
        buttons.push_back(Button::QuitGame);
        background.load_image_data(data_path("RatWin.png"), OriginLocation::UpperLeftOrigin);
        break;

    case MenuType::StartMenu:
        title = "Cheese on a roll";
        title_x_pos = 220.0f;
        buttons.push_back(Button::Play);
        buttons.push_back(Button::QuitGame);
        buttons.push_back(Button::Instructions);
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
        if (b.handle_click(evt, window_size, last_drawable_size))
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
    // Display to all buffers and stop drawing
    // if (displayed < 10)
    // {
        // displayed++;

        last_drawable_size = drawable_size;

        glUseProgram(lit_color_texture_program->program);

        GLuint lights = 1u; // number of lights
        glUniform1ui(lit_color_texture_program->LIGHTS_uint, lights);
        // glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);

        int type = 3;
        glUniform1iv(lit_color_texture_program->LIGHT_TYPE_int_array, 1, &type);

        glm::vec3 dir(0.0f, 0.0f, -1.0f);
        glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3_array, 1, glm::value_ptr(dir));

        glm::vec3 energy(1.0f, 1.0f, 0.95f);
        glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3_array, 1, glm::value_ptr(energy));

        /* glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
         glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));*/
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

        if (rank != NO_RANK)
        {
            text.draw_text("Score", drawable_size, glm::vec2(10.0f, drawable_size.y / 4.0f), glm::vec3(1.0f, 1.0f, 1.0f));
            numbers.draw_text(std::to_string(score), drawable_size, glm::vec2(drawable_size.x / 2.0f, drawable_size.y / 4.0f), glm::vec3(1.0f, 1.0f, 1.0f));

            text.draw_text("Rank", drawable_size, glm::vec2(10.0f, drawable_size.y / 2.0f), glm::vec3(1.0f, 1.0f, 1.0f));
            rankSprites[rank].create_mesh(Mode::window, 0.1f, 0.0f, 0.4f);
            rankSprites[rank].draw_mesh();
        }

        text.draw_text(title, drawable_size, glm::vec2(drawable_size.x / 2.0f - title_x_pos, 70.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    // }
}