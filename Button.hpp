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

struct Button
{
    glm::vec2 position = glm::vec2(0.0f);
    float width = 0.0f, height = 0.0f;

    UIElement button;

    void (*callback)(void) = nullptr;

    bool handle_click(SDL_Event const &, glm::uvec2 const &window_size);

    void draw(glm::uvec2 const &drawable_size);

    void update(float elapsed);

    static Button MainMenu;
    static Button Resume;
    static Button Play;
    static Button QuitGame;

    glm::uvec2 last_drawable_px = glm::uvec2(1, 1);

    Button() = default; // Default constructor
    Button(void (*_callback)(void), UIElement _button, glm::vec2 position, float height);
};