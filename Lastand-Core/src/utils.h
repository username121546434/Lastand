#pragma once
#include <array>
#include <cstdint>
#include <ostream>
#include <vector>

struct Color {
    uint8_t r, g, b, a;
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    std::ios_base::fmtflags f( os.flags() );  // save flags state
    os << "vec{ " << std::hex;
    for (auto i: v)
        os << (int)i << ' ';
    os << '}';
    os.flags(f);

    return os;
}

template <typename T, typename std::size_t n>
std::ostream &operator<<(std::ostream &os, const std::array<T, n> &v) {
    std::ios_base::fmtflags f( os.flags() );  // save flags state
    os << "arr{ " << std::hex;
    for (auto i: v)
        os << (int)i << ' ';
    os << '}';
    os.flags(f);

    return os;
}

bool is_within(int a, int b, double c);

