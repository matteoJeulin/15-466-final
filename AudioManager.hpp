#pragma once

#include "Sound.hpp"
#include <glm/glm.hpp>

namespace AudioManager {
	void init();

	void play_cheese_jump(
		float volume = 1.0f,
		float pan = 0.0f
	);
}