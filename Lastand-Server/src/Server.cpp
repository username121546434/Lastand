#include <enet/enet.h>
#include <iostream>
#include <string>
#include "constants.h"
#include "Player.h"
#include <map>
#include <vector>

int players_connected {0};
const int max_players = 100;
const Player default_player {0, 0, {0, 0, 0, 255}, "Player"};

void send_packet(ENetPeer *peer, const std::string &data, int channel_id) {
    std::cout << "Sending packet: " << data << std::endl;
    ENetPacket *packet = enet_packet_create(data.c_str(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    int val = enet_peer_send(peer, channel_id, packet);
    if (val != 0) {
        std::cerr << "Failed to send packet: " << val << std::endl;
        enet_packet_destroy(packet);
    }
}

void broadcast_packet(ENetHost *server, const std::string &data) {
    ENetPacket *packet = enet_packet_create(data.c_str(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
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

    std::map<int, Player> players;
    int new_player_id {0};

    bool running = true;
    ENetEvent event;
    std::cout << "hosting on port " << address.port << std::endl;
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
                players[new_player_id] = p;
                event.peer->data = &players.at(new_player_id);
                send_packet(event.peer, p.username + std::to_string(p.x) + std::to_string(p.y) + std::to_string(p.color.r) + std::to_string(p.color.g) + std::to_string(p.color.b), channel_user_updates);
                new_player_id++;
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                std::vector<short> data;
                for (int i {0}; i < event.packet->dataLength; i++)
                    data.push_back(event.packet->data[i]);
                std::cout << "A packet of length " << event.packet->dataLength
                        << " containing \"" << data << "\" "
                        << "was received from " << event.peer->address.host << ':' << event.peer->address.port << " "
                        << "from channel " << static_cast<int>(event.channelID) << std::endl;
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << event.peer->address.host << ':' << event.peer->address.port << " disconnected." << std::endl;
                players_connected--;
                break;
        }
    }

    enet_host_destroy(server);
}
