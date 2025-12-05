#pragma once

#include "Mode.hpp"
#include "UIElement.hpp"
#include "TextManager.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <cmath>

struct Button;

struct MenuMode : Mode
{
    enum MenuType
    {
        StartMenu,
        WinMenu,
        LoseMenu,
        LevelClearMenu
    };

    enum Rank
    {
        S,
        A,
        B,
        C,
        D,
        F,
        NO_RANK
    };

    static std::unordered_map<Rank, UIElement> rankSprites;

    MenuMode(MenuType menu, Rank _rank = NO_RANK, int _score = -1);
    virtual ~MenuMode();

    // functions called by main loop:
    virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    virtual void update(float elapsed) override;
    virtual void draw(glm::uvec2 const &drawable_size) override;

    std::vector<Button> buttons;
    UIElement background;
    glm::uvec2 last_drawable_size = glm::uvec2(0, 0);

    TextManager text = TextManager(data_path("Fonts/Monteilga.otf").c_str(), 54);
    TextManager numbers = TextManager(data_path("Fonts/Royal_Cocktail.ttf").c_str(), 60);

    std::string title;
    float title_x_pos;

    // Score keeping
    int score = -1;
    Rank rank = NO_RANK;
    bool background_mesh_created = false;

    int displayed = 0;
};