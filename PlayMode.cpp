#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>

Load<StateMachine> story_states(LoadTagDefault, []() -> StateMachine const *
								{
	std::vector<StateMachine::State> states;

	std::ifstream file("./parsing/test.story", std::ios::binary);
	read_chunk(file, "stry", &states);

	return new StateMachine(states); });

PlayMode::PlayMode()
{
	story = StateMachine(story_states.value);
	story.reset();
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (evt.key.key == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_LEFT)
		{
			left.pressed = false;
			return false;
		}
		else if (evt.key.key == SDLK_RIGHT)
		{
			right.pressed = false;
			return false;
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	{
		if (left.pressed)
		{
			left.pressed = false;
			story.switch_state(story.current_state.transitions[0]);
		}
		else if (right.pressed)
		{
			right.pressed = false;
			story.switch_state(story.current_state.transitions[1]);
		}
	}

	// reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

	{
		story.draw_current_state(drawable_size, glm::vec2(36.0f, 36.0f), glm::vec3(1.0f, 1.0f, 1.0f));
	}
	GL_ERRORS();
}
