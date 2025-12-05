#include "ControlsMenu.hpp"
#include "Mode.hpp"

#include "Button.hpp"

size_t ControlsMenu::currentIndex = 0;
std::vector<UIElement> ControlsMenu::controls = std::vector<UIElement>();
std::shared_ptr<Mode> ControlsMenu::nextMode = nullptr;

void nextPage()
{
    ControlsMenu::currentIndex++;

    if (ControlsMenu::currentIndex >= ControlsMenu::controls.size())
        Mode::set_current(ControlsMenu::nextMode);
}

ControlsMenu::ControlsMenu(std::shared_ptr<Mode> _nextMode, Type type)
{
    currentIndex = 0;
    nextMode = _nextMode;

    controls = std::vector<UIElement>();

    if (type == Type::Controls) {
        UIElement instructions;
        instructions.load_image_data(data_path("Instructions.png"), OriginLocation::UpperLeftOrigin);
        //secondPage.load_image_data(data_path("RatWin.png"), OriginLocation::UpperLeftOrigin);
        controls.emplace_back(instructions);
    }
    else if (type == Type::Level1Cutscene) {
        UIElement firstPage;
        UIElement secondPage;
        firstPage.load_image_data(data_path("Cutscenes/cutscene_1.png"), OriginLocation::UpperLeftOrigin);
        secondPage.load_image_data(data_path("Cutscenes/cutscene_2.png"), OriginLocation::UpperLeftOrigin);
        controls.emplace_back(firstPage);
        controls.emplace_back(secondPage);
    }
    else {
        UIElement lvl2cutscene;
        lvl2cutscene.load_image_data(data_path("Cutscenes/cutscene_3.png"), OriginLocation::UpperLeftOrigin);
        //secondPage.load_image_data(data_path("RatWin.png"), OriginLocation::UpperLeftOrigin);
        controls.emplace_back(lvl2cutscene);
    }


    UIElement continueButton;
    continueButton.load_image_data(data_path("Buttons/continue_button.png"), OriginLocation::UpperLeftOrigin);

    buttons.emplace_back(Button(&nextPage, continueButton, glm::vec2(0.0f, -0.7f), 0.2f));
}

bool ControlsMenu::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
    for (Button b : buttons)
    {
        if (b.handle_click(evt, window_size, last_drawable_size))
            return true;
    }

    return false;
}

void ControlsMenu::update(float elapsed)
{
    for (Button &b : buttons)
    {
        b.update(elapsed);
    }
}

void ControlsMenu::draw(glm::uvec2 const &drawable_size)
{
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

    if (controls[currentIndex].data_created)
    {
        controls[currentIndex].create_mesh(Mode::window, 0.0f, 0.0f, 2.0f);
        controls[currentIndex].draw_mesh();
    }

    for (Button b : buttons)
    {
        b.draw(drawable_size);
    }
}