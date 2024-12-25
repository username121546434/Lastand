#include "serialize.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif // _WIN32

std::pair<uint8_t, uint8_t> serialize_uint16(uint16_t val) {
    uint16_t network_value = htons(val); // Convert to network byte order

    uint8_t high_byte = static_cast<uint8_t>((network_value & 0xFF00) >> 8);
    uint8_t low_byte = static_cast<uint8_t>(network_value & 0x00FF);

    return std::make_pair(high_byte, low_byte);
}

uint16_t deserialize_uint16(uint8_t high_byte, uint8_t low_byte) {
    uint16_t network_value = (high_byte << 8) | low_byte;
    return ntohs(network_value);
}

std::array<uint8_t, 4> serialize_color(Color color) {
    std::array<uint8_t, 4> result;
    result[0] = color.r;
    result[1] = color.g;
    result[2] = color.b;
    result[3] = color.a;
    return result;
}

std::array<uint8_t, 4> serialize_coordinates(uint16_t x, uint16_t y) {
    std::array<uint8_t, 4> result;
    auto player_x {serialize_uint16(x)};
    auto player_y {serialize_uint16(y)};

    result[0] = player_x.first;
    result[1] = player_x.second;
    result[2] = player_y.first;
    result[3] = player_y.second;
    return result;
}

std::vector<uint8_t> serialize_player(const Player &player) {
    std::vector<uint8_t> result;
    result.reserve(sizeof(Player));

    auto coordinates {serialize_coordinates(player.x, player.y)};
    for (auto c : coordinates) {
        result.push_back(c);
    }

    result.push_back(player.id);

    auto color {serialize_color(player.color)};
    for (auto c : color) {
        result.push_back(c);
    }

    if (player.username.size() > 255) {
        std::cerr << "Username too long to serialize! username: " << player.username << std::endl;
        throw std::runtime_error("Username is too long to serialize!");
    }
    uint8_t username_length = static_cast<uint8_t>(player.username.size());
    result.push_back(username_length);
    for (uint8_t c: player.username) {
        result.push_back(c);
    }

    return result;
}

Player deserialize_player(const std::vector<uint8_t> &data) {
    if (data.size() <= 10) {
        std::cerr << "Not enough data to deserialize a player, data: " << data << std::endl;
        throw std::runtime_error("Not enough data to deserialize a player");
    }

    Player p {data[4]};
    uint16_t x = deserialize_uint16(data[0], data[1]);
    uint16_t y = deserialize_uint16(data[2], data[3]);
    p.x = x;
    p.y = y;

    p.color = {data[5], data[6], data[7], data[8]};
    
    uint8_t username_length = data[9];
    if (username_length + 9 != data.size() - 1) {
        std::cerr << "Warning: Username length mismatch: " << (int)username_length + 9 << " vs " << data.size() - 1 << std::endl;
    }
    std::string username;
    for (size_t i {10}; i < data.size(); i++) {
        username.push_back(data[i]);
    }
    p.username = username;
    return p;
}


std::array<uint8_t, 12> serialize_obstacle(const Obstacle &obstacle) {
    std::array<uint8_t, 12> result;

    auto [high_byte, low_byte] = serialize_uint16(obstacle.x);
    result[0] = high_byte;
    result[1] = low_byte;

    std::tie(high_byte, low_byte) = serialize_uint16(obstacle.y);
    result[2] = high_byte;
    result[3] = low_byte;

    std::tie(high_byte, low_byte) = serialize_uint16(obstacle.width);
    result[4] = high_byte;
    result[5] = low_byte;

    std::tie(high_byte, low_byte) = serialize_uint16(obstacle.height);
    result[6] = high_byte;
    result[7] = low_byte;

    auto color_data = serialize_color(obstacle.color);
    result[8] = color_data[0];
    result[9] = color_data[1];
    result[10] = color_data[2];
    result[11] = color_data[3];

    return result;
}


Obstacle deserialize_obstacle(const std::array<uint8_t, 12> &data) {
    Obstacle result;

    uint8_t high_byte = data[0];
    uint8_t low_byte = data[1];
    result.x = deserialize_uint16(high_byte, low_byte);

    high_byte = data[2];
    low_byte = data[3];
    result.y = deserialize_uint16(high_byte, low_byte);

    high_byte = data[4];
    low_byte = data[5];
    result.width = deserialize_uint16(high_byte, low_byte);

    high_byte = data[6];
    low_byte = data[7];
    result.height = deserialize_uint16(high_byte, low_byte);

    result.color.r = data[8];
    result.color.g = data[9];
    result.color.b = data[10];
    result.color.a = data[11];

    return result;
}

void update_player_delta(ClientMovement movement, bool key_up, std::pair<short, short> &player_delta) {
    auto m = static_cast<uint8_t>(movement);
    if (!key_up) {
        // key is currently being held down
        if (m & (uint8_t)ClientMovement::Left)
            player_delta.first = -1;
        else if (m & (uint8_t)ClientMovement::Right)
            player_delta.first = 1;
        
        if (m & (uint8_t)ClientMovement::Up)
            player_delta.second = -1;
        else if (m & (uint8_t)ClientMovement::Down)
            player_delta.second = 1;
    } else {
        if (m & (uint8_t)ClientMovement::Right || m & (uint8_t)ClientMovement::Left)
            player_delta.first = 0;

        if (m & (uint8_t)ClientMovement::Up || m & (uint8_t)ClientMovement::Down)
            player_delta.second = 0;
    }
}

// takes in a vector of players that were updated by the server and serializes them
std::vector<uint8_t> serialize_game_player_positions(const std::vector<Player> &players) {
    std::vector<uint8_t> result;
    result.reserve(players.size() * 5 + 1);
    if (players.size() > 255) {
        std::cerr << "Player vector too big to serialize!" << players.size() << std::endl;
        return {};
    }
    result.push_back(static_cast<uint8_t>(players.size()));
    for (const auto &p: players) {
        result.push_back(p.id);
        auto coordinates {serialize_coordinates(p.x, p.y)};
        for (auto c : coordinates) {
            result.push_back(c);
        }
    }
    return result;
}

void deserialize_and_update_game_player_positions(const std::vector<uint8_t> &data, std::map<int, Player> &players) {
    if (data.size() < 1) return;
    uint8_t num_players = data[0];
    if (data.size() != num_players * 5 + 1) {
        std::cerr << "Not enough data to deserialize players, data:" << data << std::endl;
        return;
    }

    for (size_t curr_player = 1; curr_player <= data.size() - 5; curr_player += 5) {
        int id = data[curr_player];
        auto &p = players[id];
        uint16_t x = deserialize_uint16(data[curr_player + 1], data[curr_player + 2]);
        uint16_t y = deserialize_uint16(data[curr_player + 3], data[curr_player + 4]);
        p.x = x;
        p.y = y;
    }
}

