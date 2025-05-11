#pragma once

#include <glm/glm.hpp>

#include "ray.hpp"

struct Material;

struct alignas(64) HitInfo {
    Ray r;
    glm::vec3 triangle[3];
    glm::vec3 normal;
    glm::vec2 uv;
    float t;
    Material* mat_ptr;
};