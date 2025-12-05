#pragma once

#include "Mode.hpp"
#include "UIElement.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <cmath>

struct Button;

struct ControlsMenu : Mode
{
    enum Type
    {
        Controls,
        Level1Cutscene,
        Level2Cutscene
    };

    ControlsMenu(std::shared_ptr<Mode> _nextMode, Type type);

    // functions called by main loop:
    virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    virtual void update(float elapsed) override;
    virtual void draw(glm::uvec2 const &drawable_size) override;


    std::vector<Button> buttons;
    static std::vector<UIElement> controls;
    static size_t currentIndex;
    static std::vector<bool> controls_mesh_created;

    // Mode to go to after cycling through the instructions
    static std::shared_ptr<Mode> nextMode;

    glm::uvec2 last_drawable_size = glm::uvec2(0, 0);
};