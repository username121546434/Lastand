#include "utils.h"

bool is_within(int a, int b, double c) {
    int d = std::abs(a - b);
    return d <= c;
}

Color random_color() {
    return {static_cast<uint8_t>(rand() % 200 + 56), static_cast<uint8_t>(rand() % 200 + 56), static_cast<uint8_t>(rand() % 200 + 56), 255};
}

