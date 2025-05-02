//
// Created by Rafayel Gardishyan on 24/04/2025.
//

#ifndef TRIANGULAR_LIGHT_H
#define TRIANGULAR_LIGHT_H
#include "color.h"
#include <glm/vec3.hpp>
#include <vector>
#include "lib/tiny_bvh.h"
#include "tiny_bvh_types.h"

class triangular_light {
public:
    glm::vec3 v0, v1, v2; // Vertices of the triangle
    glm::vec3 normal;    // Normal vector of the triangle
    glm::vec3 c;     // Color of the light
    float intensity; // Intensity of the light

    triangular_light (const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 c, const float intensity)
        : v0(v0), v1(v1), v2(v2), c(c), intensity(intensity) {
        // Calculate the normal vector of the triangle
        const glm::vec3 edge1 = v1 - v0;
        const glm::vec3 edge2 = v2 - v0;
        normal = glm::normalize(cross(edge1, edge2));
    }


    float area() const {
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        return 0.5f * glm::length(glm::cross(edge1, edge2));
    }
};

inline glm::vec3 sample_on_light(const triangular_light &light) {
    // Sample a random point on the triangle
    float r1 = float(rand()) / RAND_MAX;
    float r2 = float(rand()) / RAND_MAX;
    float sqrt_r1 = std::sqrt(r1);
    float u = 1 - sqrt_r1;
    float v = r2 * sqrt_r1;
    return (1 - u - v) * light.v0 + u * light.v1 + v * light.v2;
}

inline int add_light_to_triangle_soup(
        std::vector<tinybvh::bvhvec4> &triangle_soup,
        const triangular_light &light) {
    int index_offset = triangle_soup.size();
    triangle_soup.emplace_back(toBVHVec(light.v0));
    triangle_soup.emplace_back(toBVHVec(light.v1));
    triangle_soup.emplace_back(toBVHVec(light.v2));
    return index_offset;
}

#endif //TRIANGULAR_LIGHT_H
