#pragma once

#include <glm/vec3.hpp>
#include <vector>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif

#include "tiny_bvh_types.hpp"
#include "geometry.hpp"


class Light {
public:
    glm::vec3 c;     // Color of the light
    float intensity; // Intensity of the light

    Light(const glm::vec3 c, const float intensity);

    virtual float area() const = 0;
    virtual glm::vec3 normal(const glm::vec2& uv) const = 0;
    virtual glm::vec3 normal(const glm::vec3& light_pos) const = 0;
    virtual glm::vec3 sample_on_light(float& pdf) const = 0;
    virtual glm::vec3 sample_direction(const glm::vec3& point, float& pdf) const = 0;
};

class PointLight : public Light {
public:
    glm::vec3 position; // Position of the point light

    PointLight(const glm::vec3 c, const float intensity, const glm::vec3 position, const glm::vec3 normal);
    PointLight(const glm::vec3 c, const glm::vec3 position, const glm::vec3 normal);

    float area() const override;
    glm::vec3 normal(const glm::vec2& uv) const override;
    glm::vec3 normal(const glm::vec3& light_pos) const override;
    glm::vec3 sample_on_light(float& pdf) const override;
    glm::vec3 sample_direction(const glm::vec3& point, float& pdf) const override;
private:
    glm::vec3 _normal; // Normal vector of the point light
};

class TriangularLight : public Light {
public:
    Triangle triangle; // Triangle representing the light

    TriangularLight(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 c, const float intensity);
    TriangularLight(const Triangle triangle, const glm::vec3 c, const float intensity);

    float area() const override;
    glm::vec3 normal(const glm::vec2& uv) const override;
    glm::vec3 normal(const glm::vec3& light_pos) const override;
    glm::vec3 sample_on_light(float& pdf) const override;
    glm::vec3 sample_direction(const glm::vec3& point, float& pdf) const override;

};

inline int add_light_to_triangle_soup(
    std::vector<Triangle>& triangle_soup,
    const TriangularLight& light) {
    int index_offset = triangle_soup.size();
    triangle_soup.emplace_back(light.triangle);
    return index_offset;
}

glm::vec3 cosine_weighted_hemisphere_sample(const glm::vec3& normal, float& pdf);

glm::vec2 calculate_uv(const Triangle& triangle, const glm::vec3& point);