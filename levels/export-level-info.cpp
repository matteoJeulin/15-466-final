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

Level load_level(std::string inputFileName) {
    std::ifstream levelTextFile;
    levelTextFile.open(inputFileName.c_str());
    Level level;

    if (levelTextFile.is_open()) {
        std::string line;
        int lineNum = 1;
        int blockLineCounter = 0;

        while (getline(levelTextFile, line)) {
            if (line.length() > 0 && (&line[0])[0] != '%') {
                if (lineNum == 2) { // Level Rank and Spawn
                    std::stringstream linestream(line);
                    std::vector<std::string> info = {};
                    std::string piece;

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

                    std::cout << "Level " << inputFileName << ": S = " << level.s_rank_time << ", "
                                                           << ": A = " << level.a_rank_time << ", "
                                                           << ": B = " << level.b_rank_time << ", "
                                                           << ": C = " << level.c_rank_time << ", "
                                                           << ": D = " << level.d_rank_time << ", "
                                                           << ": Spawn Transform = " << level.spawnLocation << "\n\n";
                }
                else { // Camera Blocks
                    std::stringstream linestream(line);
                    std::vector<std::string> info = {};
                    std::string piece;

                    while (getline(linestream, piece, ',')) {
                        info.emplace_back(piece);
                    }

                    CameraBlock *block = &level.cameraBlocks[blockLineCounter / 3];
                    if (blockLineCounter % 3 == 0) {
                        // std::cout << "0" << "\n";
                        block->playerLeft = std::stof(info[0]);
                        block->playerRight = std::stof(info[1]);
                        block->playerBottom = std::stof(info[2]);
                        block->playerTop = std::stof(info[3]);

                        // std::cout << "Level " << inputFileName <<
                        //              " Player Range of Block: (" << block->playerLeft << "," << block->playerBottom << ") -> (" <<
                        //              block->playerRight << "," << block->playerTop << ")\n";
                    }
                    else if (blockLineCounter % 3 == 1) {
                        // std::cout << "1" << "\n";
                        block->cameraLeft = std::stof(info[0]);
                        block->cameraRight = std::stof(info[1]);
                        block->cameraBottom = std::stof(info[2]);
                        block->cameraTop = std::stof(info[3]);

                        // std::cout << "Level " << inputFileName <<
                        //              "Camera Range of Block: (" << block->cameraLeft << "," << block->cameraBottom << ") -> (" <<
                        //              block->cameraRight << "," << block->cameraTop << ")" << "\n";
                    }
                    else {
                        block->cameraVerticalOffset = std::stof(info[0]);

                        std::cout << "Level " << inputFileName <<
                                     " Player Range of Block: (" << block->playerLeft << "," << block->playerBottom << ") -> (" <<
                                     block->playerRight << "," << block->playerTop << ")\n" <<
                                     "Camera Range of Block: (" << block->cameraLeft << "," << block->cameraBottom << ") -> (" <<
                                     block->cameraRight << "," << block->cameraTop << ")" << "\n" <<
                                     "Camera Vertical Offset: " << block->cameraVerticalOffset << "\n\n";
                    }

                    blockLineCounter++;
                }
            }
            lineNum++;
        }
        level.numberOfCameraBlocks = (blockLineCounter / 3) + 1;
    }
    levelTextFile.close();

    return level;
}

int main() {
    std::vector<Level> levels = {};
    std::ofstream levelOutput;
    levelOutput.open("../dist/levels.lvl", std::ios::binary);

    levels.emplace_back(load_level("level1_info.txt"));
    std::cout << std::endl;
    levels.emplace_back(load_level("level2_info_wip.txt"));

    levelOutput.write(reinterpret_cast<const char*>(&levels[0]), levels.size() * sizeof(Level));
    levelOutput.close();

    /*{
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
    }
    */

    return 0;
}