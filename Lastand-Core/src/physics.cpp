#include "physics.h"
#include "serialize.h"
#include <cmath>
#include <iostream>

bool is_within(int a, int b, double c) {
    int d = std::abs(a - b);
    return d <= c;
}

CollisionResult detect_collision(const Player& player, const Obstacle& obstacle) {
    // Player bounds
    uint16_t player_right = player.x + (player_size * 2);
    uint16_t player_bottom = player.y + (player_size * 2);
    uint16_t player_left = player.x;
    uint16_t player_top = player.y;

    // Obstacle bounds
    uint16_t obstacle_right = obstacle.x + (obstacle.width * 2);
    uint16_t obstacle_bottom = obstacle.y + (obstacle.height * 2);
    uint16_t obstacle_left = obstacle.x;
    uint16_t obstacle_top = obstacle.y;

    // Check for touch
    bool touch = false;

    // Determine axis of touch
    CollisionAxis axis = CollisionAxis::None;
    ClientMovement cm = ClientMovement::None;

    if ((player_bottom >= obstacle_top && obstacle_bottom <= player_bottom) ||
        (player_top >= obstacle_top && player_top <= obstacle_bottom)) {
        touch = true;
        axis = CollisionAxis::Vertical;
        if (is_within(player_right, obstacle_left, 1)) {
            cm = ClientMovement::Left;
        } else if (is_within(player_left, obstacle_right, 1)) {
            cm = ClientMovement::Right;
        } else {
            axis = CollisionAxis::None;
            touch = false;
        }
    } else if ((player_left <= obstacle_right && obstacle_left <= player_left) ||
                (player_right <= obstacle_right && player_right >= obstacle_left)) {
        touch = true;
        axis = CollisionAxis::Horizontal;
        if (is_within(player_bottom, obstacle_top, 1)) {
            cm = ClientMovement::Up;
        } else if (is_within(player_top, obstacle_bottom, 1)) {
            cm = ClientMovement::Down;
        } else {
            axis = CollisionAxis::None;
            touch = false;
        }
    }

    // Check for overlap
    bool overlap =
        (player_left < obstacle_right && player_right > obstacle_left) ||
         (player_top < obstacle_bottom && player_bottom > obstacle_top);

    std::cout << "Player: " << player.x << ", " << player.y << " Obstacle: (" << obstacle.x << ", " << obstacle.y
              << ") (" << obstacle.width << ", " << obstacle.height << ") Touch: " << touch << " Axis: " << (int)axis
              << " ClientMovement: " << (int)cm << " Overlap: " << overlap << std::endl;

    return {touch, axis, overlap, cm};
}
