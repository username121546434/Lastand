#pragma once
#ifndef PROJECTILE_H
#define PROJECTILE_H
#include <cstdint>

struct Projectile {
    uint16_t x;
    uint16_t y;
    int32_t dx; // either 1 or -1
    int32_t dy;
};

struct ProjectileDouble {
    double x;
    double y;
    double dy;

    ProjectileDouble(Projectile p) : x(p.x), y(p.y), dy(static_cast<double>(p.dy) / p.dx) {}
};

#endif // PROJECTILE_H
