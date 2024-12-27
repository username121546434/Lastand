#include "obstacle.h"
#include "Player.h"
#include "serialize.h"

enum class CollisionAxis {
    None,
    Horizontal,
    Vertical,
    Both
};

struct CollisionResult {
    bool touch;                 // Whether the player and obstacle touch
    CollisionAxis axis;         // Axis of collision, if any
    bool overlap;               // Whether they overlap
    ClientMovement allowed_directions;
};

bool detect_collision(const Player& player, const std::vector<Obstacle>& obstacles);

