#pragma once

#include "Sound.hpp"
#include <glm/glm.hpp>

namespace AudioManager {
	void init();

	enum class Event {
		CheeseRegularJump,
		CheeseWeakJump,
		CheeseStrongJump,
		CheeseMelt,
		CheeseResolidify,
		StoveClick,
		MouseSqueak,
		WineHit,
		WineTimer
	};

	void play_event(Event e, float volume = 1.0f, float pan = 0.0f);
}