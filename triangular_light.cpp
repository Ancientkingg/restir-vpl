#include "triangular_light.hpp"

#include <glm/glm.hpp>
#include <vector>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif

#include "tiny_bvh_types.hpp"


TriangularLight::TriangularLight(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 c, const float intensity)
    : triangle(v0, v1, v2, -1), c(c), intensity(intensity) {
    pdf = 1;
}

TriangularLight::TriangularLight(const Triangle triangle, const glm::vec3 c, const float intensity)
	: triangle(triangle), c(c), intensity(intensity) {
	pdf = 1;
}


float TriangularLight::area() const {
    glm::vec3 edge1 = triangle.v1.position - triangle.v0.position;
    glm::vec3 edge2 = triangle.v2.position - triangle.v0.position;
    return 0.5f * glm::length(glm::cross(edge1, edge2));
}

inline glm::vec3 sample_on_light(const TriangularLight& light);

inline int add_light_to_triangle_soup(
    std::vector<Triangle>& triangle_soup,
    const TriangularLight& light);