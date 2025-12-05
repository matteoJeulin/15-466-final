#include "MovingWall.hpp"
#include <numbers>
#include <iostream>

MovingWall::MovingWall(float amplitude, float period, float offset) {
    a = amplitude;
    if (period <= 0) {
        std::cout << "WARNING: non-positive period given, setting period to 1...\n";
        b = 2.0f * (float)(std::numbers::pi);
    }
    else
        b = 2.0f * (float)(std::numbers::pi) / period;

    c = offset;
    t = 0.0f;
}

void MovingWall::update(float elapsed) {
    while (t >= 2.0f * (float)(std::numbers::pi)) {
        t -= 2.0f * (float)(std::numbers::pi);
    }
    collision->position.z = a * std::cos(b * t) + c;
    t += elapsed;
}