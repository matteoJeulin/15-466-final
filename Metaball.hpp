#pragma once

#include "Scene.hpp"
#include "SoftBody.hpp"
#include "MetaballProgram.hpp"

// Renders the cheese metaball volume inside the cube specified by cheese_xform.
//
void draw_cheese_metaballs(
    SoftBody const &soft,
    Scene::Camera const &camera,
    Scene::Transform const &cheese_xform,
    float metaball_time
);