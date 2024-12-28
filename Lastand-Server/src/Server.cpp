#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <enet/enet.h>
#include <ios>
#include <iostream>
#include <ostream>
#include <string>
#include "Projectile.h"
#include "constants.h"
#include "Player.h"
#include "serialize.h"
#include <map>
#include <utility>
#include <vector>
#include <chrono>
#include "physics.h"
#include "utils.h"

int players_connected {0};
const int max_players = 100;
const Player default_player {0, 0, {255, 255, 255, 255}, "Player", 0};

// the most top left the player can go
constexpr uint16_t min_x {0};
constexpr uint16_t min_y {0};

// the most bottom right the player can go
constexpr uint16_t max_x {(window_size - player_size) * 2};
constexpr uint16_t max_y {(window_size - player_size) * 2};

struct ClientData {
    Player p;
    std::pair<short, short> player_movement;
};

std::ostream &operator<<(std::ostream &os, const ENetAddress &e) {
    os << e.host << ':' << e.port;
    return os;
}

void send_packet(ENetPeer *peer, const std::vector<uint8_t> &data, int channel_id) {
    std::cout << "Sending packet: " << data << '\n';
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    int val = enet_peer_send(peer, channel_id, packet);
    if (val != 0) {
        std::cerr << "Failed to send packet: " << val << " to: " << peer->address << std::endl;
        enet_packet_destroy(packet);
    }
}

void broadcast_packet(ENetHost *server, const std::vector<uint8_t> &data, int channel_id) {
    std::cout << "Broadcasting packet: " << data << '\n';
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, channel_id, packet);
}

void parse_client_move(const ENetEvent &event) {
    ClientData &cd {*static_cast<ClientData *>(event.peer->data)};
    ClientMovementTypes movement_type {event.packet->data[1]};
    ClientMovement movement {event.packet->data[2]};
    switch (movement_type) {
        case ClientMovementTypes::Start:
            update_player_delta(movement, false, cd.player_movement);
            break;
        case ClientMovementTypes::Stop:
            update_player_delta(movement, true, cd.player_movement);
            break;
        default:
            std::cerr << "Client movement type not recognized: " << (int)movement_type << std::endl;
    }
    std::cout << "Client movement updated to: " << cd.player_movement.first << ", " << cd.player_movement.second << '\n';
}

void parse_client_shoot(const ENetEvent &event, std::vector<ProjectileDouble> &projectiles) {
    assert(event.packet->dataLength == 13);
    std::array<uint8_t, 12> projectile_data {
        event.packet->data[1],
        event.packet->data[2],
        event.packet->data[3],
        event.packet->data[4],
        event.packet->data[5],
        event.packet->data[6],
        event.packet->data[7],
        event.packet->data[8],
        event.packet->data[9],
        event.packet->data[10],
        event.packet->data[11],
        event.packet->data[12],
    };
    Projectile p {deserialize_projectile(projectile_data)};
    ProjectileDouble pd {p};

    std::cout << "Shooting projectile: " << pd.x << ", " << pd.y << ", " << p.dx << ", " << p.dy << '\n';

    projectiles.push_back(pd);
}

void parse_event(const ENetEvent &event, std::vector<ProjectileDouble> &projectiles) {
    if (event.channelID == channel_updates) {
        MessageToServerTypes event_type {event.packet->data[0]};
        assert(event_type == MessageToServerTypes::ClientMove || event_type == MessageToServerTypes::Shoot);

        if (event_type == MessageToServerTypes::ClientMove){
            parse_client_move(event);
        } else if (event_type == MessageToServerTypes::Shoot) {
            parse_client_shoot(event, projectiles);
        }
    }
}

void run_game_tick(std::map<int, ClientData> &players, const std::vector<Obstacle> &obstacles, std::vector<ProjectileDouble> &projectiles) {
    for (auto &[id, data] : players) {
        if (data.player_movement == std::make_pair<short, short>(0, 0))
            continue;
        if ((data.p.x <= min_x && data.player_movement.first == -1) ||
            (data.p.x >= max_x && data.player_movement.first == 1)) {
            data.player_movement.first = 0;
        }
        if ((data.p.y <= min_y && data.player_movement.second == -1) ||
            (data.p.y >= max_y && data.player_movement.second == 1)) {
            data.player_movement.second = 0;
        }
        if (data.player_movement == std::make_pair<short, short>(0, 0))
            continue;
        Player test_px {data.p};
        test_px.move(std::make_pair(data.player_movement.first, 0));
        auto collision_x = detect_collision(test_px, obstacles);

        Player test_py {data.p};
        test_py.move(std::make_pair(0, data.player_movement.second));
        auto collision_y = detect_collision(test_py, obstacles);

        std::cout << "Collision x: " << collision_x << ", Collision y: " << collision_y << '\n';
        auto actual_movement = std::make_pair(data.player_movement.first, data.player_movement.second);

        if (collision_x)
            actual_movement.first = 0;
        if (collision_y)
            actual_movement.second = 0;
        data.p.move(actual_movement);
        if (actual_movement != std::make_pair<short, short>(0, 0))
            std::cout << "Player moved to " << id << ": " << data.p.x << ", " << data.p.y << '\n';
    }
    std::vector<uint16_t> projectiles_to_remove;
    projectiles_to_remove.reserve(projectiles.size());
    uint16_t idx = 0;
    for (auto &p : projectiles) {
        p.move();
        if (p.x > max_x || p.y > max_y || p.x < min_x || p.y < min_y) {
            projectiles_to_remove.push_back(idx);
        }
        idx++;
    }
    for (auto idx : projectiles_to_remove)
        projectiles.erase(projectiles.begin() + idx);
}

int main(int argv, char **argc) {
    if (enet_initialize() != 0) {
        std::cerr << "Couldn't initialize enet" << std::endl;
        return 1;
    }
    std::atexit(enet_deinitialize);
    std::cout << std::boolalpha;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 8888;
    if (argv > 1)
        address.port = std::stoi(argc[1]);

    ENetHost *server {enet_host_create(&address, max_players, num_channels, 0, 0)};
    if (server == NULL) {
        std::cerr << "Couldn't initialize ENetHost" << std::endl;
        return 1;
    }

    std::map<int, ClientData> players;
    std::vector<ProjectileDouble> projectiles;
    int new_player_id {0};

    bool running = true;
    ENetEvent event;
    std::cout << "hosting on port " << address.port << std::endl;

    // map3 kind of looks cool
    // map5 has a big wall
    const std::vector<Obstacle> obstacles {load_from_file("maps/map3.txt")};
    std::cout << "Loaded " << obstacles.size() << " obstacles" << std::endl;
    bool sent_empty_projectiles = false;

#if defined(DEBUG)
    for (const auto &o : obstacles) {
        std::cout << "Read obstacle at: (" << o.x << ", " << o.y << ") (" << o.width << ", " << o.height << ")"
            << "(" << (int)o.color.r << ", " << (int)o.color.g << ", " << (int)o.color.b << ", " << (int)o.color.a << ")" << std::endl;
        auto data = serialize_obstacle(o);
        std::cout << "Correct obstacle serialized: " << data << std::endl;
    }
#endif

    auto last_time = std::chrono::high_resolution_clock::now();

    while (running) {
        int err = enet_host_service(server, &event, tick_rate_ms);
        if (err < 0) {
            std::cerr << "An error occurred in enet" << std::endl;
        }

        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::cout << "A new client connected from: " << event.peer->address.host << ':' << event.peer->address.port << std::endl;
                players_connected++;
                Player p {default_player};
                p.username += std::to_string(new_player_id);
                p.id = new_player_id;
                ClientData c {p, {0, 0}};
                players[new_player_id] = c;
                event.peer->data = &players.at(new_player_id);

                std::vector<uint8_t> broadcast_data = serialize_player(p);
                broadcast_data.insert(broadcast_data.cbegin(), static_cast<uint8_t>(MessageToClientTypes::PlayerJoined));
                broadcast_packet(server, broadcast_data, channel_events);

                std::cout << "Sending previous game data to player " << new_player_id << std::endl;

                std::vector<Player> other_players;
                for (const auto &[id, data] : players) {
                    if(id == new_player_id)
                        continue;
                    other_players.push_back(data.p);
                }
                std::vector<uint8_t> previous_game_data {serialize_previous_game_data(other_players, obstacles)};

#ifdef DEBUG
                // testing if serializing and deserializing previous game data works
                auto [p2, o2] = deserialize_and_update_previous_game_data(previous_game_data);
                if (p2.size() != other_players.size()) {
                    std::cerr << "slkdjflskdf" << std::endl;
                }
                if (o2.size() != obstacles.size()) {
                    std::cerr << "slkdjflskdf obstacles" << std::endl;
                }
                for (size_t i {0}; i < p2.size(); i++) {
                    auto p1 {other_players[i]};
                    auto p3 {p2[i]};
                    if (p1.id != p3.id || p1.username != p3.username || p1.x != p3.x || p1.y != p3.y || p1.color.r != p3.color.r || p1.color.g != p3.color.g || p1.color.b != p3.color.b || p1.color.a != p3.color.a)
                        std::cerr << "slkdjflskdf player is different " << std::endl;
                }
                for (size_t i {0}; i < o2.size(); i++) {
                    auto o1 {obstacles[i]};
                    auto o3 {o2[i]};
                    if (o1.x != o3.x || o1.y != o3.y || o1.width != o3.width || o1.height != o3.height || o1.color.r != o3.color.r || o1.color.g != o3.color.g || o1.color.b != o3.color.b || o1.color.a != o3.color.a)
                        std::cerr << "slkdjflskdf obstacle is different " << std::endl;
                }
#endif

                previous_game_data.insert(previous_game_data.begin(), static_cast<uint8_t>(MessageToClientTypes::PreviousGameData));

                send_packet(event.peer, previous_game_data, channel_events);

                new_player_id++;
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                std::vector<short> data;
                for (int i {0}; i < event.packet->dataLength; i++)
                    data.push_back(event.packet->data[i]);
#ifdef DEBUG
                std::cout << "A packet of length " << event.packet->dataLength
                        << " containing \"" << data << "\" "
                        << "was received from " << event.peer->address << " "
                        << "from channel " << static_cast<int>(event.channelID) << std::endl;
#endif
                parse_event(event, projectiles);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                std::cout << event.peer->address.host << ':' << event.peer->address.port << " disconnected." << std::endl;
                players_connected--;
                ClientData *c = static_cast<ClientData *>(event.peer->data);
                std::vector<uint8_t> broadcast_data {static_cast<uint8_t>(MessageToClientTypes::PlayerLeft), c->p.id};
                players.erase(players.find(c->p.id));

                broadcast_packet(server, broadcast_data, channel_events);
                break;
            }
            case ENET_EVENT_TYPE_NONE:
                break;
        }
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
        if (elapsed_time_ms >= tick_rate_ms || is_within(elapsed_time_ms, tick_rate_ms, 1)) {
            last_time = now;
            run_game_tick(players, obstacles, projectiles);

            std::vector<Player> players_to_update;
            players_to_update.reserve(players.size());
            for (const auto &[id, player_data]: players) {
                if (player_data.player_movement == std::make_pair<short, short>(0, 0))
                    continue;
                players_to_update.push_back(player_data.p);
            }

            if (!players_to_update.empty()) {
                std::vector<uint8_t> data_to_send {serialize_game_player_positions(players_to_update)};
                data_to_send.insert(data_to_send.cbegin(), static_cast<uint8_t>(MessageToClientTypes::UpdatePlayerPositions));
                broadcast_packet(server, data_to_send, channel_updates);
            }

            if (!projectiles.empty() || !sent_empty_projectiles) {
                std::vector<uint8_t> projectile_data;
                projectile_data.reserve(2 + projectiles.size() * sizeof(Projectile));
                projectile_data.push_back(static_cast<uint8_t>(MessageToClientTypes::UpdateProjectiles));
                projectile_data.push_back(static_cast<uint8_t>(projectiles.size()));
                for (auto &pd: projectiles) {
                    Projectile p {static_cast<uint16_t>(pd.x), static_cast<uint16_t>(pd.y), pd.dx, static_cast<int32_t>(pd.dy)};
                    auto p_data = serialize_projectile(p);
                    projectile_data.insert(projectile_data.end(), p_data.cbegin(), p_data.cend());
                }
                broadcast_packet(server, projectile_data, channel_updates);
                if (projectiles.empty())
                    sent_empty_projectiles = true;
                else
                    sent_empty_projectiles = false;
            }
        }
    }

    enet_host_destroy(server);
}
