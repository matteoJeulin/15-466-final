/********************************************************************************
 * Allows for music loops with unique intros (right now only supports 2D sounds)
 * Functionality based on Jim McCann's provided Sound.hpp and Sound.cpp files
 ********************************************************************************/
#pragma once

#include "Sound.hpp"
using namespace std;
// #include <string>
// #include "data_path.hpp"

struct DynamicSoundLoop {
    Sound::Sample *first_pass;
    Sound::Sample *loop_pass;
    std::shared_ptr< Sound::PlayingSample > my_playing_sample;
    bool playing_sample_is_valid = false;

    float play_volume;
    float pan;
    bool playing = false;
    bool on_loop = false;

    // DynamicSoundLoop(std::string first_, std::string loop_) : first_pass(first_), loop_pass(loop_) {
    //     my_playing_sample = nullptr;
    //     on_loop = false;
    // };

    DynamicSoundLoop(Sound::Sample *first_, Sound::Sample *loop_) : first_pass(first_), loop_pass(loop_) {
        my_playing_sample = nullptr;
        on_loop = false;
    };

    void play(float pv_, float pan_) {      
        play_volume = pv_;
        pan = pan_;

        playing = true;
        my_playing_sample = Sound::play(*first_pass, play_volume, pan);
    };

    void update() {
        if (!on_loop && playing) {
            assert(my_playing_sample != nullptr);
            if (my_playing_sample->stopped) {
                playing_sample_is_valid = false;
                my_playing_sample = Sound::loop(*loop_pass, play_volume, pan);
                on_loop = true;
            }
        }
    };

    std::shared_ptr< Sound::PlayingSample > playing_sample(bool *valid) {
        playing_sample_is_valid = true;
        valid = &playing_sample_is_valid;

        return my_playing_sample;
    }

    /***********
     * Setters!
     ***********/
    void set_volume(float new_volume, float ramp) {
        play_volume = new_volume;
        my_playing_sample->set_volume(new_volume, ramp);
    }

    void set_pan(float new_pan, float ramp) {
        pan = new_pan;
        my_playing_sample->set_pan(new_pan, ramp);
    }

    ~DynamicSoundLoop() {
        my_playing_sample->stop();
    }
};