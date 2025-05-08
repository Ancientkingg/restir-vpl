#include "triangular_light.hpp"

#include <glm/glm.hpp>
#include <vector>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif

#include "tiny_bvh_types.hpp"


TriangularLight::TriangularLight(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 c, const float intensity)
    : v0(v0), v1(v1), v2(v2), c(c), intensity(intensity) {
    // Calculate the normal vector of the triangle
    const glm::vec3 edge1 = v1 - v0;
    const glm::vec3 edge2 = v2 - v0;
    normal = glm::normalize(cross(edge1, edge2));
    pdf = 1;
}


float TriangularLight::area() const {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    return 0.5f * glm::length(glm::cross(edge1, edge2));
}

inline glm::vec3 sample_on_light(const TriangularLight& light);

inline int add_light_to_triangle_soup(
    std::vector<tinybvh::bvhvec4>& triangle_soup,
    const TriangularLight& light);