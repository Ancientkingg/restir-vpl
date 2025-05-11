#pragma once

#include <glm/glm.hpp>

#include "ray.hpp"
#include "geometry.hpp"

struct Material;

struct alignas(64) HitInfo {
    Triangle triangle;
    Ray r;
    glm::vec2 uv;
    float t;
    Material* mat_ptr;
};