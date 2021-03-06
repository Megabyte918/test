#pragma once
#include "Entity.hpp"

class Virus : public Entity {
public:
    static const int TYPE = 1;

    Virus(const Vec2&, float radius, const Color&) noexcept;
    void split(double angle, float radius) noexcept;
    void onDespawned() noexcept;
    void consume(e_ptr prey) noexcept;
    ~Virus();
};