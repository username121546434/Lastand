#pragma once
#ifndef PROJECTILE_H
#define PROJECTILE_H
#include <cstdint>

struct Projectile {
    uint16_t x;
    uint16_t y;
    int8_t dx; // either 1 or -1
    int32_t dy;
};

struct ProjectileDouble {
    double x;
    double y;
    int8_t dx; // either 1 or -1
    double dy;

    ProjectileDouble(Projectile p) : x(p.x), y(p.y), dx(p.dx), dy(p.dy) {}
};

#endif // PROJECTILE_H
