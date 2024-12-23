#include <SDL3/SDL.h>
#include "Player.h"
#include <iostream>
#include <SDL3/SDL_main.h>
#include "SDL3/SDL_scancode.h"
#include "constants.h"
#include <enet/enet.h>
#include <vector>
#include "serialize.h"

void draw_player(SDL_Renderer *renderer, const Player &p) {
    SDL_FRect frect {static_cast<float>(p.x / 2.0), static_cast<float>(p.y / 2.0), player_size, player_size};
    bool success;
    success = SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.g, p.color.a);
    if (!success) std::cerr << "Error in SDL_SetRenderDrawColor: " << SDL_GetError();
    success = SDL_RenderFillRect(renderer, &frect);
    if (!success) std::cerr << "Error in SDL_RenderFillRect: " << SDL_GetError();
}

const std::string window_title {"Lastand Client"};

void draw_frame(SDL_Renderer *renderer, const Player &player) {
    draw_player(renderer, player);
}

ClientMovement create_client_movement(SDL_Scancode key) {
    ClientMovement m;
    switch (key) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            m = ClientMovement::Up;
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            m = ClientMovement::Down;
            break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
            m = ClientMovement::Left;
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            m = ClientMovement::Right;
            break;
        default:
            m = ClientMovement::None;
    }
    return m;
}

std::vector<uint8_t> handle_key_down(SDL_Scancode key, std::pair<short, short> &player_delta) {
    auto movement = create_client_movement(key);
    std::vector<uint8_t> msg {
        static_cast<uint8_t>(MessageToServerTypes::ClientMove),
        static_cast<uint8_t>(ClientMovementTypes::Start),
        static_cast<uint8_t>(movement)
    };
    update_player_delta(movement, false, player_delta);
    return msg;
}

std::vector<uint8_t> handle_key_up(SDL_Scancode key, std::pair<short, short> &player_delta) {
    auto movement = create_client_movement(key);
    std::vector<uint8_t> msg {
        static_cast<uint8_t>(MessageToServerTypes::ClientMove),
        static_cast<uint8_t>(ClientMovementTypes::Stop),
        static_cast<uint8_t>(movement)
    };
    update_player_delta(movement, true, player_delta);
    return msg;
}

std::vector<uint8_t> process_event(const SDL_Event &event, std::pair<short, short> &player_delta) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            return handle_key_down(event.key.scancode, player_delta);
        case SDL_EVENT_KEY_UP:
            return handle_key_up(event.key.scancode, player_delta);
        default:
            return {};
    }
}

void run_game_tick(Player &player, std::pair<short, short> player_delta) {
    player.move(player_delta);
}

template <typename T>
void send_packet(ENetPeer *peer, const T &data, int channel_id) {
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    int val = enet_peer_send(peer, channel_id, packet);
    if (val != 0) {
        std::cerr << "Failed to send packet: " << val << std::endl;
        enet_packet_destroy(packet);
    }
}

int main(int argv, char **argc) {
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing Enet!" << std::endl;
        return 1;
    }

    ENetHost *client {enet_host_create(NULL, 1, num_channels, 0, 0)};
    if (client == NULL) {
        std::cerr << "An error occured while creating ENetHost" << std::endl;
        return EXIT_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL failed to initialize: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window {nullptr};
    SDL_Renderer *renderer {nullptr};
    SDL_CreateWindowAndRenderer(window_title.c_str(), 600, 600, SDL_WINDOW_MAXIMIZED, &window, &renderer);

    if (!window || window == NULL) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (!renderer || renderer == NULL) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::string server_addr {"127.0.0.1"};
    int port {8888};

    for (int i {0}; i < argv; i++)
        std::cout << argc[i] << std::endl;

    if (argv == 3) {
        server_addr = argc[1];
        port = std::stoi(argc[2]);
    } else if (argv == 2)
        port = std::stoi(argc[1]);

    ENetAddress address;
    ENetEvent enet_event;
    
    enet_address_set_host(&address, server_addr.c_str());
    address.port = port;
    ENetPeer *server {enet_host_connect(client, &address, num_channels, 0)};
    if (server == NULL) {
        std::cerr << "Failed to connect to peer" << std::endl;
        return EXIT_FAILURE;
    }

    if (enet_host_service(client, &enet_event, 5000) > 0 && enet_event.type == ENET_EVENT_TYPE_CONNECT) {
        std::cout << "Connection to " << server_addr << ":" << address.port << " success" << std::endl;
    } else {
        enet_peer_reset(server);
        std::cout << "Connection to " << server_addr << ":" << address.port << " failed" << std::endl;
    }

    Player player {100, 100, {100, 200, 0, 0}, "testing...", 0};
    std::pair<short, short> player_movement;
    Uint32 lastTime = SDL_GetTicks();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            auto old_player_movement {player_movement};
            auto data_to_send {process_event(event, player_movement)};
            if (!data_to_send.empty() && player_movement != old_player_movement) {
                send_packet(server, data_to_send, channel_updates);
            }
        }

        while (enet_host_service(client, &enet_event, tick_rate_ms) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    std::string data;
                    for (int i {0}; i < enet_event.packet->dataLength; i++)
                        data.push_back(enet_event.packet->data[i]);
                    std::cout << "Received data: " << data << " on channel: " << enet_event.channelID << std::endl;
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (SDL_GetTicks() - lastTime >= tick_rate_ms) {
            run_game_tick(player, player_movement);
            lastTime = SDL_GetTicks();
        }

        draw_frame(renderer, player);

        SDL_RenderPresent(renderer);
    }


    enet_peer_disconnect(server, 0);

    while (enet_host_service(client, &enet_event, 3000) > 0) {
        switch (enet_event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(enet_event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnet event received" << std::endl;
                break;
        }
    }

    std::cout << "Disconnected from server" << std::endl;

    enet_deinitialize();
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    return 0;
}
