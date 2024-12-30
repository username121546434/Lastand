#include <SDL3/SDL.h>
#include "Obstacle.h"
#include "Player.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <SDL3/SDL_main.h>
#include "Projectile.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "constants.h"
#include <enet/enet.h>
#include <tuple>
#include <utility>
#include <vector>
#include "serialize.h"
#include <map>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_stdlib.h"

void draw_player(SDL_Renderer *renderer, const Player &p) {
    SDL_FRect frect {static_cast<float>(p.x / 2.0), static_cast<float>(p.y / 2.0), player_size, player_size};
    bool success;
    success = SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.g, p.color.a);
    if (!success) std::cerr << "Error in SDL_SetRenderDrawColor: " << SDL_GetError();
    success = SDL_RenderFillRect(renderer, &frect);
    if (!success) std::cerr << "Error in SDL_RenderFillRect: " << SDL_GetError();
}

void draw_obstacle(SDL_Renderer *renderer, const Obstacle &o) {
    SDL_FRect frect {
        static_cast<float>(o.x / 2.0),
        static_cast<float>(o.y / 2.0),
        static_cast<float>(o.width),
        static_cast<float>(o.height)
    };
    // std::cout << "Drawing obstacle at: (" << o.x << ", " << o.y << ") (" << o.width << ", " << o.height << ")" << std::endl;
    bool success = SDL_SetRenderDrawColor(renderer, o.color.r, o.color.g, o.color.b, o.color.a);
    if (!success) std::cerr << "Error in SDL_SetRenderDrawColor: " << SDL_GetError();
    success = SDL_RenderFillRect(renderer, &frect);
    if (!success) std::cerr << "Error in SDL_RenderFillRect: " << SDL_GetError();
}

void draw_projectile(SDL_Renderer *renderer, const Projectile &p) {
    SDL_FRect frect {
        static_cast<float>(p.x / 2.0),
        static_cast<float>(p.y / 2.0),
        2.0, 2.0
    };
    bool success = SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    if (!success) std::cerr << "Error in SDL_SetRenderDrawColor: " << SDL_GetError();
    success = SDL_RenderFillRect(renderer, &frect);
    if (!success) std::cerr << "Error in SDL_RenderFillRect: " << SDL_GetError();
}

const std::string window_title {"Lastand Client"};

void draw_frame(SDL_Renderer *renderer, const std::vector<Player> &players) {
    for (const auto &player: players)
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

std::vector<uint8_t> handle_key_down(SDL_Scancode key) {
    auto movement = create_client_movement(key);
    std::vector<uint8_t> msg {
        static_cast<uint8_t>(MessageToServerTypes::ClientMove),
        static_cast<uint8_t>(ClientMovementTypes::Start),
        static_cast<uint8_t>(movement)
    };
    return msg;
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

std::vector<uint8_t> handle_mouse_up(uint16_t x, uint16_t y, SDL_MouseButtonEvent event) {
    y = static_cast<uint16_t>(y + player_size);
    x = static_cast<uint16_t>(x + player_size);
    Projectile p {x, y, static_cast<int32_t>(event.x * 2) - x, static_cast<int32_t>(event.y * 2) - y};
    std::cout << "Projectile: (" << p.x << ", " << p.y << ")(" << p.dx << ", " << p.dy << ")\n";
    auto data = serialize_projectile(p);
    std::vector<uint8_t> msg {
        static_cast<uint8_t>(MessageToServerTypes::Shoot)
    };
    msg.insert(msg.end(), data.begin(), data.end());
    return msg;
}

std::vector<uint8_t> process_event(const SDL_Event &event, std::pair<short, short> &player_delta, uint16_t player_x, uint16_t player_y) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            return handle_key_down(event.key.scancode, player_delta);
        case SDL_EVENT_KEY_UP:
            return handle_key_up(event.key.scancode, player_delta);
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return handle_mouse_up(player_x, player_y, event.button);
        default:
            return {};
    }
}

void parse_message_from_server(const std::vector<uint8_t> &data, std::map<int, Player> &player_data, std::vector<Projectile> &projectiles) {
    MessageToClientTypes type {data[0]};
    std::vector<uint8_t> data_without_type {data.begin() + 1, data.end()};
    switch (type) {
        case MessageToClientTypes::UpdatePlayerPositions: {
            std::cout << "update player positions" << std::endl;
            deserialize_and_update_game_player_positions(data_without_type, player_data);
            break;
        }
        case MessageToClientTypes::PlayerJoined: {
            std::cout << "Player joined" << std::endl;
            Player p {deserialize_player(data_without_type)};
            player_data[p.id] = p;
            break;
        }
        case MessageToClientTypes::PlayerLeft: {
            int id {data_without_type[0]};
            std::cout << "Player " << id << " left" << std::endl;
            player_data.erase(id);
            break;
        }
        case MessageToClientTypes::UpdateProjectiles: {
            projectiles.clear();
            for (size_t i = 1, proj = 0; i < data_without_type.size() && proj < data_without_type[0]; i += sizeof(Projectile), proj++) {
                std::array<uint8_t, 12> data {
                    data_without_type[i],
                    data_without_type[i + 1],
                    data_without_type[i + 2],
                    data_without_type[i + 3],
                    data_without_type[i + 4],
                    data_without_type[i + 5],
                    data_without_type[i + 6],
                    data_without_type[i + 7],
                    data_without_type[i + 8],
                    data_without_type[i + 9],
                    data_without_type[i + 10],
                    data_without_type[i + 11]
                };
                projectiles.push_back(deserialize_projectile(data));
            }
            break;
        }
        case MessageToClientTypes::PlayerKilled: {
            assert(data_without_type.size() == 2);
            uint8_t killer {data_without_type[0]};
            uint8_t killed {data_without_type[1]};
            std::cout << player_data.at(killer).username << " has killed " << player_data.at(killed).username << std::endl;
            player_data.erase(killed);
        }
        case MessageToClientTypes::GameStarted: {
            std::cout << "The game has started!" << std::endl;
        }
    }
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

// gets the player that is this client
Player get_this_player(ENetHost *client) {
    ENetEvent event;
    int err = enet_host_service(client, &event, 800);
    if (err < 0) {
        std::cerr << "Failed to get player data: " << err << std::endl;
        std::exit(1);
    }
    Player this_player;
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
        std::vector<uint8_t> vec(event.packet->data + 1, event.packet->data + event.packet->dataLength);
        std::cout << "Data received: " << *(int*)event.packet->data << " and " << vec << std::endl;
        this_player = deserialize_player(vec);
        std::cout << "Received player: " << this_player.username << ", ("
                  << this_player.x << ", " << this_player.y << "), (" << (int)this_player.color.r << ','
                  << (int)this_player.color.g << ',' << (int)this_player.color.b << ',' << (int)this_player.color.a << "):"
                  << (int)this_player.id << std::endl;
    } else {
        std::cerr << "Did not receive player data: " << event.type << std::endl;
        std::exit(1);
    }
    return this_player;
}

std::pair<std::map<int, Player>, std::vector<Obstacle>> get_previous_game_data(ENetHost *client) {
    ENetEvent event;
    int err = enet_host_service(client, &event, 800);
    if (err < 0) {
        std::cerr << "Failed to get player data: " << err << std::endl;
        std::exit(1);
    }
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
        std::vector<uint8_t> vec(event.packet->data + 1, event.packet->data + event.packet->dataLength);
        std::cout << "Data received: " << *(int*)event.packet->data << " and " << vec << std::endl;
        auto [previous_players, obstacles] = deserialize_and_update_previous_game_data(vec);
        std::cout << "Received " << previous_players.size() << " player(s) and " << obstacles.size() << " obstacle(s)" << std::endl;
        return std::make_pair(previous_players, obstacles);
    } else {
        std::cerr << "Did not receive previous game data: " << event.type << std::endl;
        std::exit(1);
    }
}

std::tuple<Player, std::map<int, Player>, std::vector<Obstacle>, ENetPeer*> connect_to_server(ENetHost *client, const std::string &server_addr, int port) {
    ENetAddress address;
    ENetEvent enet_event;
    
    enet_address_set_host(&address, server_addr.c_str());
    address.port = port;
    ENetPeer *server {enet_host_connect(client, &address, num_channels, 0)};
    if (server == NULL) {
        std::cerr << "Failed to connect to peer" << std::endl;
        std::exit(1);
    }

    if (enet_host_service(client, &enet_event, 5000) > 0 && enet_event.type == ENET_EVENT_TYPE_CONNECT) {
        std::cout << "Connection to " << server_addr << ":" << address.port << " success" << std::endl;
    } else {
        enet_peer_reset(server);
        std::cout << "Connection to " << server_addr << ":" << address.port << " failed" << std::endl;
    }

    // get player data
    Player this_player = get_this_player(client);

    // get previous game data
    auto [players, obstacles] = get_previous_game_data(client);
    return {this_player, players, obstacles, server};
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
    SDL_CreateWindowAndRenderer(window_title.c_str(), window_size, window_size, SDL_WINDOW_MAXIMIZED, &window, &renderer);

    if (!window || window == NULL) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (!renderer || renderer == NULL) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io {ImGui::GetIO()};
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    std::string server_addr;
    int port {};

    for (int i {0}; i < argv; i++)
        std::cout << argc[i] << std::endl;

    if (argv == 3) {
        server_addr = argc[1];
        port = std::stoi(argc[2]);
    } else if (argv == 2)
        port = std::stoi(argc[1]);


    std::pair<short, short> player_movement;
    std::vector<Projectile> projectiles;
    ImVec4 player_color {1.0f, 1.0f, 1.0f, 1.0f};
    char username[15] = "";
    bool connected_to_server = false;

    // the player the client is controlling
    Player local_player;
    std::map<int, Player> players;
    ENetPeer *server {nullptr};
    std::vector<Obstacle> obstacles;

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                running = false;
            else if (connected_to_server) {
                auto last_movement = player_movement;
                std::vector<uint8_t> data_to_send {process_event(event, player_movement, players.at(local_player.id).x, players.at(local_player.id).y)};
                if (!data_to_send.empty() && (player_movement != last_movement || event.type == SDL_EVENT_MOUSE_BUTTON_UP)) {
                    send_packet(server, data_to_send, channel_updates);
                }
            }
        }
        
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (!connected_to_server) {
            ImGui::Begin("Enter your details");
            ImGui::ColorEdit4("Choose your color", (float*)&player_color);
            ImGui::InputTextWithHint("Username", "Enter username", username, 15);
            ImGui::InputTextWithHint("Input server address", "Enter server address", &server_addr);
            ImGui::InputInt("Input server port", &port);

            std::cout << "Username: " << username << std::endl;
            std::cout << "Server address: " << server_addr << std::endl;
            std::cout << "Port: " << port << std::endl;
            if (ImGui::Button("Connect to the server")) {
                connected_to_server = true;
                std::tie(local_player, players, obstacles, server) = connect_to_server(client, server_addr, port);
                players[local_player.id] = local_player;
                // send username and color to server
                std::vector<uint8_t> color_change {
                    static_cast<uint8_t>(MessageToServerTypes::SetClientAttributes),
                    static_cast<uint8_t>(SetClientAttributesTypes::Color),
                    static_cast<uint8_t>(player_color.x * 255),
                    static_cast<uint8_t>(player_color.y * 255),
                    static_cast<uint8_t>(player_color.z * 255),
                    static_cast<uint8_t>(player_color.w * 255)
                };
                send_packet(server, color_change, channel_user_updates);
                std::vector<uint8_t> username_change {
                    static_cast<uint8_t>(MessageToServerTypes::SetClientAttributes),
                    static_cast<uint8_t>(SetClientAttributesTypes::Username),
                    static_cast<uint8_t>(strlen(username))
                };
                username_change.insert(username_change.end(), username, username + strlen(username));
                send_packet(server, username_change, channel_user_updates);
            }
            ImGui::End();
        } else {
            ENetEvent enet_event;
            while (enet_host_service(client, &enet_event, tick_rate_ms) > 0) {
                switch (enet_event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    std::vector<uint8_t> data;
                    for (int i{0}; i < enet_event.packet->dataLength; i++)
                        data.push_back(enet_event.packet->data[i]);
                    std::cout << "Received data: " << data << " on channel: " << (int)enet_event.channelID << '\n';
                    parse_message_from_server(data, players, projectiles);
                    break;
                }
                default:
                    break;
                }
            }
        }


        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        for (const auto &[id, player] : players)
            draw_player(renderer, player);
        
        for (const auto &obstacle : obstacles)
            draw_obstacle(renderer, obstacle);

        for (auto p : projectiles)
            draw_projectile(renderer, p);

        SDL_RenderPresent(renderer);
    }


    enet_peer_disconnect(server, 0);
    ENetEvent enet_event;
    while (enet_host_service(client, &enet_event, 500) > 0) {
        switch (enet_event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(enet_event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnet event received" << std::endl;
                break;
            default:
                break;
        }
    }

    std::cout << "Disconnected from server" << std::endl;

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui::DestroyContext();

    // enet_deinitialize();
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    return 0;
}
