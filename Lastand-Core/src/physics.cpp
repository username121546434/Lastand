#include "physics.h"

bool collision(const Obstacle &o1, const Player &o2) {
    if (o1.x + o1.width < o2.x)
        return false;
    if (o2.x + player_size < o1.x)
        return false;
    if (o1.y + o1.height < o2.y)
        return false;
    if (o2.y + player_size < o1.y)
        return false;
    return true;
}
