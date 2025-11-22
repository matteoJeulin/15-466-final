#include "Mode.hpp"

std::shared_ptr< Mode > Mode::current;
std::shared_ptr< Mode > Mode::previous;

SDL_Window *Mode::window = NULL;

void Mode::set_current(std::shared_ptr< Mode > const &new_current) {
	previous = current;
	current = new_current;
	//NOTE: may wish to, e.g., trigger resize events on new current mode.
}
