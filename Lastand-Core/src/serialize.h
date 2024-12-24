#pragma once
#include <vector>
#ifndef SERIALIZE_H
#define SERIALIZE_H
#include <cstdint>
#include "Player.h"
#include <map>

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

enum class MessageToClientTypes: uint8_t {
    // player positions have changed, sent on channel_updates.
    // data from serialize_game_player_positions() should be after this
    UpdatePlayerPositions = 0,

    // player attributes (username or color) have changed
    SetPlayerAttributes = 1,
    PlayerDied = 2, // player has died
    PlayerLeft = 3, // player has left
    PlayerJoined = 4, // player has joined
    PlayerWon = 5, // player has won
};

enum class SetPlayerAttributesTypes: uint8_t {
    UsernameChanged = 0,
    ColorChanged = 1
};

std::vector<uint8_t> serialize_player(const Player &player);
Player deserialize_player(const std::vector<uint8_t> &data);

void update_player_delta(ClientMovement movement, bool key_up, std::pair<short, short> &player_delta);

std::vector<uint8_t> serialize_game_player_positions(const std::vector<Player> &players);
void deserialize_and_update_game_player_positions(const std::vector<uint8_t> &data, std::map<int, Player> &players);

#endif
