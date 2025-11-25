#include "../Level.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <ranges>
#include <string_view>
#include <iomanip>
#include <iostream>
#include <cassert>

/*********************************************************************************************************************
 * Based on Kenechukwu's Game 3 Export-Chart script.
 * 
 * Tutorials used:
 * File reading: https://cplusplus.com/doc/tutorial/files/
 * String stream: https://www.geeksforgeeks.org/cpp/stringstream-c-applications/
 * Boost split: https://www.geeksforgeeks.org/cpp/boostsplit-c-library/
 * Writing to binary: https://stackoverflow.com/questions/14089266/how-to-correctly-write-vector-to-binary-file-in-c
 ********************************************************************************************************************/
int main() {
    std::ifstream levelTextFile;
    levelTextFile.open("level_ranks_and_spawn.txt");
    std::ofstream levelOutput;
    levelOutput.open("../dist/ranks_and_spawns.lvl", std::ios::binary);

    std::vector<Level> levels = {};
    if (levelTextFile.is_open()) {
        std::string line;
        while (getline(levelTextFile, line)) {
            if (line.length() > 0 && (&line[0])[0] != '%') {
                std::stringstream linestream(line);
                std::vector<std::string> info = {};
                std::string piece;

                Level level;

                while (getline(linestream, piece, ',')) {
                    info.emplace_back(piece);
                }

                assert(info.size() == 6);

                level.s_rank_time = std::stof(info[0]);
                level.a_rank_time = std::stof(info[1]);
                level.b_rank_time = std::stof(info[2]);
                level.c_rank_time = std::stof(info[3]);
                level.d_rank_time = std::stof(info[4]);
                
                for (size_t i = 0; i < info[5].length(); i++) {
                    level.spawnLocation[i] = info[5].at(i);
                }
                for (size_t i = info[5].length(); i < 100; i++) {
                    level.spawnLocation[i] = '\0';
                }

                levels.emplace_back(level);

                std::cout << "Level " << levels.size() << ": S = " << levels.back().s_rank_time << ", "
                                                       << ": A = " << levels.back().a_rank_time << ", "
                                                       << ": B = " << levels.back().b_rank_time << ", "
                                                       << ": C = " << levels.back().c_rank_time << ", "
                                                       << ": D = " << levels.back().d_rank_time << ", "
                                                       << ": Spawn Transform = " << levels.back().spawnLocation << "\n";
            }
        }
        levelOutput.write(reinterpret_cast<const char*>(&levels[0]), levels.size() * sizeof(Level));
        levelOutput.close();
    }
    levelTextFile.close();

    return 0;
}