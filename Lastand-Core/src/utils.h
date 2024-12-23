#pragma once
#include <cstdint>
#include <ostream>
#include <vector>

struct Color {
    uint8_t r, g, b, a;
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    os << "{ ";
    for (auto i: v)
        os << i << ' ';
    os << '}';
    return os;
}

