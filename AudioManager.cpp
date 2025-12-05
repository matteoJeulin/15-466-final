#include "AudioManager.hpp"
#include "data_path.hpp"
#include <vector>
#include <algorithm>
#include <iostream>

namespace {
	// random containers
	Sound::RandomSamples cheese_regular_jump;
	Sound::RandomSamples cheese_weak_jump;
	Sound::RandomSamples cheese_strong_jump;
	Sound::RandomSamples cheese_melt;
	Sound::RandomSamples cheese_resolidify;
	Sound::RandomSamples stove_click_turn;
	Sound::RandomSamples mouse_squeak;
	Sound::RandomSamples wine_hit;
	Sound::RandomSamples wine_timer;

	bool initialized = false;

	struct SFXLimit {
		int max_instances = 3;
		std::vector<std::weak_ptr<Sound::PlayingSample>> instances;
	};

	SFXLimit regular_jump_limit;
	SFXLimit weak_jump_limit;
	SFXLimit strong_jump_limit;
	SFXLimit melt_limit;
	SFXLimit resolidify_limit;
	SFXLimit stove_click_limit;
	SFXLimit mouse_squeak_limit;
	SFXLimit wine_hit_limit;
	SFXLimit wine_timer_limit;

	void play_with_limit(Sound::RandomSamples& container,
		SFXLimit& limit,
		float volume, float pan)
	{
		// get rid of finished sounds (expired weak_ptrs)
		auto& v = limit.instances;
		v.erase(std::remove_if(v.begin(), v.end(),
			[](std::weak_ptr<Sound::PlayingSample> const& w) {
				return w.expired();
			}),
			v.end());

		// if too many still active, skip
		if ((int)v.size() >= limit.max_instances) {
			return;
		}

		auto playing = container.play(volume, pan); // assumes RandomSamples::play returns shared_ptr
		if (playing) {
			v.emplace_back(playing);
		}
	}
}

namespace AudioManager {

	void init() {

		if (initialized) return;

		// regular jump
		static Sound::Sample cheese_regular_jump_1 = Sound::Sample(data_path("SFX/cheese_regular_jump_1.wav"));
		static Sound::Sample cheese_regular_jump_2 = Sound::Sample(data_path("SFX/cheese_regular_jump_2.wav"));

		// weak jump
		static Sound::Sample cheese_weak_jump_1 = Sound::Sample(data_path("SFX/cheese_weak_jump_1.wav"));

		// strong jump
		static Sound::Sample cheese_strong_jump_1 = Sound::Sample(data_path("SFX/cheese_strong_jump_1.wav"));

		// cheese melt
		static Sound::Sample cheese_melt_1 = Sound::Sample(data_path("SFX/cheese_melt_1.wav"));
		static Sound::Sample cheese_melt_2 = Sound::Sample(data_path("SFX/cheese_melt_2.wav"));

		// cheese resolidify
		static Sound::Sample cheese_resolidify_1 = Sound::Sample(data_path("SFX/cheese_resolidify_1.wav"));
		static Sound::Sample cheese_resolidify_2 = Sound::Sample(data_path("SFX/cheese_resolidify_2.wav"));
		
		// stove click
		static Sound::Sample stove_click_turn_1 = Sound::Sample(data_path("SFX/stove_click_turn_1.wav"));
		static Sound::Sample stove_click_turn_2 = Sound::Sample(data_path("SFX/stove_click_turn_2.wav"));
		static Sound::Sample stove_click_turn_3 = Sound::Sample(data_path("SFX/stove_click_turn_3.wav"));

		// mouse squeak
		static Sound::Sample mouse_squeak_1 = Sound::Sample(data_path("SFX/mouse_squeak_1.wav"));
		static Sound::Sample mouse_squeak_2 = Sound::Sample(data_path("SFX/mouse_squeak_2.wav"));
		static Sound::Sample mouse_squeak_3 = Sound::Sample(data_path("SFX/mouse_squeak_3.wav"));
		static Sound::Sample mouse_squeak_4 = Sound::Sample(data_path("SFX/mouse_squeak_4.wav"));

		// wine hit 
		static Sound::Sample wine_hit_1 = Sound::Sample(data_path("SFX/wine_bottle_hit_1.wav"));
		static Sound::Sample wine_hit_2 = Sound::Sample(data_path("SFX/wine_bottle_hit_2.wav"));
		static Sound::Sample wine_hit_3 = Sound::Sample(data_path("SFX/wine_bottle_hit_3.wav"));

		// wine timer
		static Sound::Sample wine_timer_1 = Sound::Sample(data_path("SFX/wine_bottle_timer_1.wav"));
		static Sound::Sample wine_timer_2 = Sound::Sample(data_path("SFX/wine_bottle_timer_2.wav"));
		static Sound::Sample wine_timer_3 = Sound::Sample(data_path("SFX/wine_bottle_timer_3.wav"));

		// add to random containers
		cheese_regular_jump.add(cheese_regular_jump_1);
		cheese_regular_jump.add(cheese_regular_jump_2);

		cheese_weak_jump.add(cheese_weak_jump_1);
		cheese_strong_jump.add(cheese_strong_jump_1);

		cheese_melt.add(cheese_melt_1);
		cheese_melt.add(cheese_melt_2);

		cheese_resolidify.add(cheese_resolidify_1);
		cheese_resolidify.add(cheese_resolidify_2);

		stove_click_turn.add(stove_click_turn_1);
		stove_click_turn.add(stove_click_turn_2);
		stove_click_turn.add(stove_click_turn_3);

		mouse_squeak.add(mouse_squeak_1);
		mouse_squeak.add(mouse_squeak_2);
		mouse_squeak.add(mouse_squeak_3);
		mouse_squeak.add(mouse_squeak_4);

		wine_hit.add(wine_hit_1);
		wine_hit.add(wine_hit_2);
		wine_hit.add(wine_hit_3);

		wine_timer.add(wine_timer_1);
		wine_timer.add(wine_timer_2);
		wine_timer.add(wine_timer_3);

		// set limit instances
		regular_jump_limit.max_instances = 1; 
		weak_jump_limit.max_instances = 2; 
		strong_jump_limit.max_instances = 2; 
		melt_limit.max_instances = 1;
		resolidify_limit.max_instances = 1;
		stove_click_limit.max_instances = 2;
		mouse_squeak_limit.max_instances = 4;
		wine_hit_limit = 1;
		wine_timer_limit = 1;

		std::cout << "sound effects initialized" << std::endl;

		initialized = true;
	}

	void play_event(Event e, float volume, float pan) {
		
		switch (e) {
		case Event::CheeseRegularJump:
			play_with_limit(cheese_regular_jump, regular_jump_limit, volume, pan);
			break;
		case Event::CheeseWeakJump:
			play_with_limit(cheese_weak_jump, weak_jump_limit, volume, pan);
			break;
		case Event::CheeseStrongJump:
			play_with_limit(cheese_strong_jump, strong_jump_limit, volume, pan);
			break;
		case Event::CheeseMelt:
			play_with_limit(cheese_melt, melt_limit, volume, pan);
			break;
		case Event::CheeseResolidify:
			play_with_limit(cheese_resolidify, resolidify_limit, volume, pan);
			break;
		case Event::StoveClick:
			play_with_limit(stove_click_turn, stove_click_limit, volume, pan);
			break;
		case Event::MouseSqueak:
			play_with_limit(mouse_squeak, mouse_squeak_limit, volume, pan);
			break;
		case Event::WineHit:
			play_with_limit(wine_hit, mouse_squeak_limit, volume, pan);
			break;
		case Event::WineTimer:
			play_with_limit(wine_timer, mouse_squeak_limit, volume, pan);
			break;

		default:
			break;
		}
	}
	
}