#include "Obstacle.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

std::vector<Obstacle> load_from_file(const std::string &file_name) {
    std::ifstream file {file_name};
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << file_name << std::endl;
        return {};
    }

    std::vector<Obstacle> obstacles;

    std::string curr_line;
    while (std::getline(file, curr_line)) {
        std::stringstream ss(curr_line);
        uint16_t x, y, width, height;
        uint8_t r, g, b;
        char delimiter; // To skip commas

        // Parse the line
        if (ss >> x >> delimiter >> y >> delimiter >> width >> delimiter 
               >> height >> delimiter >> r >> delimiter >> g >> delimiter >> b) {
            Obstacle obstacle{x, y, width, height, {r, g, b}};
            obstacles.push_back(obstacle);
        }
    }
    file.close();

    return obstacles;
}

