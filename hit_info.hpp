#pragma once

#include <glm/glm.hpp>

#include "ray.hpp"

struct Material;

struct alignas(64) HitInfo {
    Ray r;
    float t;
    glm::vec3 triangle[3];
    glm::vec3 normal;
    Material* mat_ptr;
    glm::vec2 uv;

    char padding[64];
};