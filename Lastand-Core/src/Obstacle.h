#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <cstdint>
#include <string>
#include "utils.h"
#include <vector>

struct Obstacle {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    Color color;
};

std::vector<Obstacle> load_from_file(const std::string &file_name);
#endif // OBSTACLE_H
