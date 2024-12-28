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
    int dx;
    double dy;

    ProjectileDouble(Projectile p) : x(p.x), y(p.y), dx(p.dx >= 0 ? (p.dx > 0 ? 1 : 0) : -1), dy(static_cast<double>(p.dy) / p.dx) {}
    void move() {
        x += dx;
        y += dy;
    }
};

#endif // PROJECTILE_H
