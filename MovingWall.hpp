#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>

#include "Scene.hpp"
#include "DynamicMeshBuffer.hpp"
#include "Mesh.hpp"

#include <vector>
#include <deque>
#include <cmath>

struct MovingWall
{
    // moves vertically, with position modelled by acos(b*t) + c
    float a; // amplitude
    float b; // 2pi / period
    float c; // vertical offset ()
    float t; // time

    Scene::Transform *collision = nullptr;

    MovingWall(float amplitude, float period, float offset);
    virtual void update(float elapsed);
};