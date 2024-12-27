#include "obstacle.h"
#include "Player.h"
#include "serialize.h"

enum class CollisionAxis {
    None,
    Horizontal,
    Vertical
};

struct CollisionResult {
    bool touch;                 // Whether the player and obstacle touch
    CollisionAxis axis;         // Axis of collision, if any
    bool overlap;               // Whether they overlap
    ClientMovement allowed_directions;
};

CollisionResult detect_collision(const Player& player, const Obstacle& obstacle);

