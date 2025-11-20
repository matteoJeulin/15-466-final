#include "AudioManager.hpp"
#include "data_path.hpp"
#include <iostream>

namespace {

	Sound::RandomSamples cheese_jump;
	bool initialized = false;
}

namespace AudioManager {

	void init() {

		if (initialized) return;

		static Sound::Sample cheese_jump_1 = Sound::Sample(data_path("jump_test_1.wav"));
		static Sound::Sample cheese_jump_2 = Sound::Sample(data_path("jump_test_2.wav"));
		static Sound::Sample cheese_jump_3 = Sound::Sample(data_path("jump_test_3.wav"));

		cheese_jump.add(cheese_jump_1);
		cheese_jump.add(cheese_jump_2);
		cheese_jump.add(cheese_jump_3);

		std::cout << "cheese jump sounds initialized" << std::endl;

		initialized = true;
	}

	void play_cheese_jump(
		float volume,
		float pan){
			cheese_jump.play(volume, pan);
		}
	
}