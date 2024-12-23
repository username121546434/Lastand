#pragma once
#include <cstdint>
#ifndef SERIALIZE_H
#define SERIALIZE_H

enum class MessageToServerTypes: uint8_t {
    ClientMove = 0, // input from player to go up, down, left, right
    SetClientAttributes = 1, // used for setting the username or color of player
};

enum class ClientMovementTypes: uint8_t {
    Start = 0, // when player presses down on an input
    Stop = 1 // when player releases an input
};

enum class ClientMovement: uint8_t {
    None  = 0b0000,

    Up    = 0b0001,
    Down  = 0b0010,
    Left  = 0b0100,
    Right = 0b1000,

    UpRight = Up | Right,
    UpLeft = Up | Left,
    DownRight = Down | Right,
    DownLeft = Down | Left,
};

#endif
