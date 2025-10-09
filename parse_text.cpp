#include "parse_text.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include "read_write_chunk.hpp"

Parser::Parser()
{
    story = {};
}

Parser::~Parser() {}

void Parser::parse_story(const std::string &filename)
{
    std::ifstream file(filename);

    // Get the number of states from the file
    uint32_t nb_states;
    file >> nb_states;

    uint32_t state_id;
    while (file >> state_id)
    {
        StateMachine::State current_state;
        current_state.id = state_id;

        char c;
        file >> c;
        // Checking proper formatting
        if (c != '{')
        {
            std::string err = "Improper formatting, was expecting '{', got '";
            err += c;
            err.append("'.");
            throw std::invalid_argument(err);
        }

        // Get the state text
        file.getline(current_state.text, max_line_length, '|');

        // Get the transitions form current_state
        uint32_t transition_index = 0;
        c = file.peek();
        while (c != '}')
        {
            assert(transition_index < max_transition && "Too many transitions from current state.");
            StateMachine::Transition t;

            file >> t.dst_id;

            // Get the transition text
            file.getline(t.text, max_line_length, '|');
            current_state.transitions[transition_index] = t;

            transition_index++;
            c = file.peek();
        }
        file >> c;

        story.emplace_back(current_state);
    }

    // assert(story.size() == nb_states && "Invalid number of states in the state machine, does not match number specified at the top of the file");

    std::string atlas = std::filesystem::path(filename).stem();
    std::ofstream out("./parsing/" + atlas + ".story", std::ios::binary);
    write_chunk("stry", story, &out);
    out.close();

    file.close();
}

// StateMachine::StateMachine()
// {
//     states = std::vector<State>();
// }

// StateMachine::StateMachine(std::vector<State> _states)
// {
//     states = std::vector(_states);
// }

int main()
{
    Parser parser;
    parser.parse_story("story.txt");
}