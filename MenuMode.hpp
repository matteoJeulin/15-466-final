#pragma once

#include "Mode.hpp"
#include "UIElement.hpp"

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
        LoseMenu
    };

    MenuMode(MenuType menu);
    virtual ~MenuMode();

    // functions called by main loop:
    virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    virtual void update(float elapsed) override;
    virtual void draw(glm::uvec2 const &drawable_size) override;

    std::shared_ptr< Mode > lastMode;

    std::vector<Button> buttons;
    UIElement background;
    glm::uvec2 last_drawable_size = glm::uvec2(0, 0);
};