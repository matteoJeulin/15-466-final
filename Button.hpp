#pragma once

#include "Mode.hpp"
#include "PlayMode.hpp"
#include "MenuMode.hpp"

#include "UIElement.hpp"

#include <glm/glm.hpp>
#include <SDL3/SDL.h>

#include <vector>
#include <deque>
#include <cmath>

struct Button {
    glm::vec2 position;
    float width, height;

    UIElement button;

    void (*callback)(void);

    bool handle_click(SDL_Event const &, glm::uvec2 const &window_size);

    void draw(glm::uvec2 const &drawable_size);

    static Button MainMenu;
    static Button Resume;
    static Button Play;
    static Button QuitGame;

    Button(void (*_callback)(void), UIElement _button);
};