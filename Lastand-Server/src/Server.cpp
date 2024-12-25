#include <cassert>
#include <cstdint>
#include <enet/enet.h>
#include <iostream>
#include <ostream>
#include <string>
#include "constants.h"
#include "Player.h"
#include "serialize.h"
#include <map>
#include <utility>
#include <vector>
#include <chrono>

int players_connected {0};
const int max_players = 100;
const Player default_player {0, 0, {255, 255, 255, 255}, "Player", 0};

struct ClientData {
    Player p;
    std::pair<short, short> player_movement;
};

std::ostream &operator<<(std::ostream &os, const ENetAddress &e) {
    os << e.host << ':' << e.port;
    return os;
}

void send_packet(ENetPeer *peer, const std::vector<uint8_t> &data, int channel_id) {
    std::cout << "Sending packet: " << data << std::endl;
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    int val = enet_peer_send(peer, channel_id, packet);
    if (val != 0) {
        std::cerr << "Failed to send packet: " << val << " to: " << peer->address << std::endl;
        enet_packet_destroy(packet);
    }
}

void broadcast_packet(ENetHost *server, const std::vector<uint8_t> &data, int channel_id) {
    std::cout << "Broadcasting packet: " << data << std::endl;
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, channel_id, packet);
}

void parse_event(const ENetEvent &event) {
    if (event.channelID == channel_updates) {
        MessageToServerTypes event_type {event.packet->data[0]};
        assert(event_type == MessageToServerTypes::ClientMove);

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
        std::cout << "Client movement updated to: " << cd.player_movement.first << ", " << cd.player_movement.second << std::endl;
    }
}

void run_game_tick(std::map<int, ClientData> &players) {
    for (auto &[id, data] : players) {
        data.p.move(data.player_movement);
        if (data.player_movement != std::make_pair<short, short>(0, 0))
            std::cout << "Player moved to " << id << ": " << data.p.x << ", " << data.p.y << std::endl;
    }
}

int main(int argv, char **argc) {
    if (enet_initialize() != 0) {
        std::cerr << "Couldn't initialize enet" << std::endl;
        return 1;
    }
    std::atexit(enet_deinitialize);

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
    int new_player_id {0};

    bool running = true;
    ENetEvent event;
    std::cout << "hosting on port " << address.port << std::endl;

    const std::vector<Obstacle> obstacles {load_from_file("maps/map1.txt")};
    std::cout << "Loaded " << obstacles.size() << " obstacles" << std::endl;

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
                previous_game_data.insert(previous_game_data.begin(), static_cast<uint8_t>(MessageToClientTypes::PreviousGameData));

                send_packet(event.peer, previous_game_data, channel_events);

                new_player_id++;
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                std::vector<short> data;
                for (int i {0}; i < event.packet->dataLength; i++)
                    data.push_back(event.packet->data[i]);
                std::cout << "A packet of length " << event.packet->dataLength
                        << " containing \"" << data << "\" "
                        << "was received from " << event.peer->address << " "
                        << "from channel " << static_cast<int>(event.channelID) << std::endl;
                parse_event(event);
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
        if (elapsed_time_ms >= tick_rate_ms) {
            run_game_tick(players);

            std::vector<Player> players_to_update;
            players_to_update.reserve(players.size());
            for (const auto &[id, player_data]: players) {
                if (player_data.player_movement == std::make_pair<short, short>(0, 0))
                    continue;
                players_to_update.push_back(player_data.p);
            }

            if (players_to_update.empty())
                continue;
            std::vector<uint8_t> data_to_send {serialize_game_player_positions(players_to_update)};
            data_to_send.insert(data_to_send.cbegin(), static_cast<uint8_t>(MessageToClientTypes::UpdatePlayerPositions));
            broadcast_packet(server, data_to_send, channel_updates);
        }
    }

    enet_host_destroy(server);
}
