#include "light.hpp"

#include <glm/glm.hpp>
#include <vector>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif
#include <random>
#include <glm/gtc/constants.hpp>

#include "tiny_bvh_types.hpp"
#include "restir.hpp"


Light::Light(const glm::vec3 c, const float intensity) : c(c), intensity(intensity) {}

PointLight::PointLight(const glm::vec3 c, const float intensity, const glm::vec3 position, const glm::vec3 normal)
    : Light(c, intensity), position(position), _normal(normal) {
}

PointLight::PointLight(const glm::vec3 c, const glm::vec3 position, const glm::vec3 normal)
    : Light(c, 1.0f), position(position), _normal(normal) {
}

float PointLight::area() const {
    //return 2.0f * glm::pi<float>(); // Point light has area of a hemisphere with radius 1
    return 1.0f;
}

glm::vec3 PointLight::normal(const glm::vec2& uv) const {
    return _normal;
}

glm::vec3 PointLight::normal(const glm::vec3& light_pos) const {
    return normal({ 0.0f, 0.0f });
}

glm::vec3 PointLight::sample_on_light(float& pdf) const {
	pdf = 1.0f; // PDF for point light is uniform
    return position; // Sample the position of the point light
}

glm::vec3 PointLight::sample_direction(const glm::vec3& p, float& pdf) const {
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float z = dist(rng) * 2.0f - 1.0f;  // [-1, 1]
    float phi = dist(rng) * glm::two_pi<float>();  // [0, 2pi)
    float r = std::sqrt(1.0f - z * z);

    float x = r * std::cos(phi);
    float y = r * std::sin(phi);

    return glm::vec3(x, y, z);
}

TriangularLight::TriangularLight(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 c, const float intensity)
    : Light(c, intensity), triangle(v0, v1, v2, -1) {
}

TriangularLight::TriangularLight(const Triangle triangle, const glm::vec3 c, const float intensity)
    : Light(c, intensity), triangle(triangle) {
}

float TriangularLight::area() const {
    glm::vec3 edge1 = triangle.v1.position - triangle.v0.position;
    glm::vec3 edge2 = triangle.v2.position - triangle.v0.position;
    return 0.5f * glm::length(glm::cross(edge1, edge2));
}

glm::vec3 TriangularLight::normal(const glm::vec2& uv) const {
    return triangle.normal(uv);
}

glm::vec3 TriangularLight::normal(const glm::vec3& light_pos) const {
    // Convert light_pos to a uv coordinate system
    glm::vec2 uv = calculate_uv(triangle, light_pos);
    glm::vec3 normal = triangle.normal(uv);

    return normal;
}

glm::vec3 TriangularLight::sample_on_light(float& pdf) const {
    // Sample a random point on the triangle
    float r1 = float(rand()) / RAND_MAX;
    float r2 = float(rand()) / RAND_MAX;
    float sqrt_r1 = std::sqrt(r1);
    float u = 1 - sqrt_r1;
    float v = r2 * sqrt_r1;
    return (1 - u - v) * triangle.v0.position + u * triangle.v1.position + v * triangle.v2.position;
}

glm::vec3 TriangularLight::sample_direction(const glm::vec3& p, float& pdf) const {
    glm::vec3 normal = triangle.normal();
    return cosine_weighted_hemisphere_sample(normal, pdf);
}

inline int add_light_to_triangle_soup(
    std::vector<Triangle>& triangle_soup,
    const TriangularLight& light);

glm::vec3 cosine_weighted_hemisphere_sample(const glm::vec3& normal, float& pdf) {
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float u1 = dist(rng);
    float u2 = dist(rng);

    float r = std::sqrt(u1);
    float theta = 2.0f * glm::pi<float>() * u2;

    float x = r * std::cos(theta);
    float y = r * std::sin(theta);
    float z = std::sqrt(1.0f - u1);  // z = cos(theta) for hemisphere

    // Create an orthonormal basis (T, B, N) from normal
    glm::vec3 N = glm::normalize(normal);
    glm::vec3 T = glm::normalize(glm::abs(N.x) > 0.1f ? glm::cross(N, glm::vec3(0, 1, 0)) : glm::cross(N, glm::vec3(1, 0, 0)));
    glm::vec3 B = glm::cross(N, T);

    return glm::normalize(x * T + y * B + z * N);
}

glm::vec2 calculate_uv(const Triangle& triangle, const glm::vec3& point) {
    glm::vec3 edge0 = triangle.v1.position - triangle.v0.position;
    glm::vec3 edge1 = triangle.v2.position - triangle.v0.position;
    glm::vec3 v0 = point - triangle.v0.position;
    float d00 = glm::dot(edge0, edge0);
    float d01 = glm::dot(edge0, edge1);
    float d11 = glm::dot(edge1, edge1);
    float d20 = glm::dot(v0, edge0);
    float d21 = glm::dot(v0, edge1);
    float denom = d00 * d11 - d01 * d01;
    if (denom == 0.0f) {
        return { 0.0f, 0.0f };
    }
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    return { 1.0f - v - w, v };
}