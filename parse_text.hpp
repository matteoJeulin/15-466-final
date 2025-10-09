#pragma once

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

// HarfBuzz
#include <hb.h>
#include <hb-ft.h>

#include <unordered_map>
#include <vector>
#include <string>

#include "StateMachine.hpp"

struct Parser
{
    struct CharacterMesh
    {
    };

    // Map of all previously seen characters and their corresponding mesh
    std::unordered_map<hb_codepoint_t, FT_Bitmap> character_atlas;
    std::vector<std::pair<hb_codepoint_t, FT_Bitmap>> character_atlas_vector;

    // State machine for the story
    std::vector<StateMachine::State> story;

    // Parse a state machine from a file.
    // The expected format is the following:
    // n
    // ...
    // i {text|j transition_text|k transition_text|...|}
    // ...
    // With i the id of the current state (states should be written in order), text the text to show
    // when arriving in state i, j the state to transition to and transition text the text to show
    // when using this transition.
    void parse_story(const std::string &filename);

    void get_meshes_from_line(char *line);

    Parser();
    ~Parser();
};
