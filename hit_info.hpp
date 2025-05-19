#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "ray.hpp"
#include "geometry.hpp"

struct Material;

struct alignas(64) HitInfo {
    Triangle triangle;
    Ray r;
    glm::vec2 uv;
    float t;
    std::weak_ptr<Material> mat_ptr;
};