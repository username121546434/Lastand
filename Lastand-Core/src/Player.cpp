#include "Player.h"

void Player::move(std::pair<short, short> delta) {
    x += delta.first;
    y += delta.second;
}

