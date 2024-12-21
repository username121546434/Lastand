#include <SDL3/SDL.h>
#include "Player.h"
#include <iostream>
#include <SDL3/SDL_main.h>
#include "constants.h"

void draw_player(SDL_Renderer *renderer, const Player &p) {
    auto [x, y] = p.get_coors();
    SDL_FRect frect {static_cast<float>(x / 10.0), static_cast<float>(y / 10.0), player_size, player_size};
    bool success;
    success = SDL_SetRenderDrawColor(renderer, p.get_color().r, p.get_color().g, p.get_color().g, p.get_color().a);
    if (!success) std::cerr << "Error in SDL_SetRenderDrawColor: " << SDL_GetError();
    success = SDL_RenderFillRect(renderer, &frect);
    if (!success) std::cerr << "Error in SDL_RenderFillRect: " << SDL_GetError();
}

const std::string window_title {"Lastand Client"};

void draw_frame(SDL_Renderer *renderer, const Player &player) {
    draw_player(renderer, player);
}

void handle_key_down(SDL_Scancode key, std::pair<short, short> &player_delta) {
    switch (key) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            player_delta.second = -1;
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            player_delta.second = 1;
            break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
            player_delta.first = -1;
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            player_delta.first = 1;
            break;
        default:
            break;
    }
}

void handle_key_up(SDL_Scancode key, std::pair<short, short> &player_delta) {
    switch (key) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            player_delta.second = 0;
            break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            player_delta.first = 0;
            break;
        default:
            break;
    }
}

void process_event(const SDL_Event &event, std::pair<short, short> &player_delta) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            handle_key_down(event.key.scancode, player_delta);
            break;
        case SDL_EVENT_KEY_UP:
            handle_key_up(event.key.scancode, player_delta);
            break;
        default:
            break;
    }
}

void run_game_tick(Player &player, std::pair<short, short> player_delta) {
    player.move(player_delta);
}

int main(int argc, char **argv) {
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

    Player player {100, 100, {100, 200, 0, 0}, "testing..."};
    std::pair<short, short> player_movement;
    Uint32 lastTime = SDL_GetTicks();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            process_event(event, player_movement);
            std::cout << "Player: " << player_movement.first << ", " << player_movement.second << std::endl;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (SDL_GetTicks() - lastTime >= tick_rate_ms) {
            run_game_tick(player, player_movement);
            Uint32 lastTime = SDL_GetTicks();
        }

        draw_frame(renderer, player);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    return 0;
}
