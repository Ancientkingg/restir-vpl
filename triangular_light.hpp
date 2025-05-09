#pragma once

#include <glm/vec3.hpp>
#include <vector>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif

#include "tiny_bvh_types.hpp"
#include "geometry.hpp"

class TriangularLight {
public:
	Triangle triangle; // Triangle representing the light
    glm::vec3 c;     // Color of the light
    float intensity; // Intensity of the light
    float pdf;

    TriangularLight(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 c, const float intensity);
	TriangularLight(const Triangle triangle, const glm::vec3 c, const float intensity);

    float area() const;
};

inline glm::vec3 sample_on_light(const TriangularLight& light) {
    // Sample a random point on the triangle
    float r1 = float(rand()) / RAND_MAX;
    float r2 = float(rand()) / RAND_MAX;
    float sqrt_r1 = std::sqrt(r1);
    float u = 1 - sqrt_r1;
    float v = r2 * sqrt_r1;
    return (1 - u - v) * light.triangle.v0.position + u * light.triangle.v1.position + v * light.triangle.v2.position;
}

inline int add_light_to_triangle_soup(
    std::vector<Triangle>& triangle_soup,
    const TriangularLight& light) {
    int index_offset = triangle_soup.size();
    triangle_soup.emplace_back(light.triangle);
    return index_offset;
}