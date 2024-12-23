#pragma once
#include <cstdint>
#include "utils.h"
#include <string>

constexpr uint8_t player_size {20};

struct Player {
    uint16_t x;
    uint16_t y;

    Color color;
    std::string username;
public:
    Player(uint16_t x, uint16_t y, Color color, const std::string &username): 
        x {x}, y {y}, color {color}, username {username} {};
    Player(): Player {0, 0, Color {}, "Player"} {};
    void move(std::pair<short, short> delta);
};

