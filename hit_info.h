#ifndef HIT_INFO_H
#define HIT_INFO_H

#include <glm/glm.hpp>

#include "ray.h"

struct material;
class Material;

struct hit_info {
    Ray r;
    float t;
    glm::vec3 triangle[3];
    glm::vec3 normal;
    material* mat;
    Material* mat_ptr;
};

#endif