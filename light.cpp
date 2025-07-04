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
    if (any(glm::isnan(c)) || any(glm::isinf(c))) {
		std::cerr << "Error: PointLight color is NaN or Inf!" << std::endl;
    }
}

PointLight::PointLight(const glm::vec3 c, const glm::vec3 position, const glm::vec3 normal)
    : Light(c, 1.0f), position(position), _normal(normal) {
}

float PointLight::area() const {
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
    return cosine_weighted_hemisphere_sample(_normal, pdf);
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

	pdf = 1.0f / area(); // PDF is uniform over the triangle area

    return (1 - u - v) * triangle.v0.position + u * triangle.v1.position + v * triangle.v2.position;
}

glm::vec3 TriangularLight::sample_direction(const glm::vec3& p, float& pdf) const {
    glm::vec3 normal = triangle.normal();
    return cosine_weighted_hemisphere_sample(normal, pdf);
}

inline int add_light_to_triangle_soup(
    std::vector<Triangle>& triangle_soup,
    const TriangularLight& light);

static void branchlessONB(const glm::vec3& n, glm::vec3& b1, glm::vec3& b2)
{
    const float sign = copysign(1.0f, n.y);
    const float a = -1.0f / (sign + n.y);
    const float b = n.x * n.z * a;
    b1 = glm::vec3(1.0f + sign * n.x * n.x * a, -sign * n.x, sign * b);
    b2 = glm::vec3(b, -n.z, sign + n.z * n.z * a);
}

static glm::vec3 rotate_vector_around_normal(const glm::vec3& normal, const glm::vec3& random_dir_local_space) {
    glm::vec3 tangent, bitangent;
    branchlessONB(normal, tangent, bitangent);
    return random_dir_local_space.x * tangent + 
           random_dir_local_space.y * normal + 
           random_dir_local_space.z * bitangent;
}

thread_local std::mt19937 rng_light(std::random_device{}());
std::uniform_real_distribution<float> dist_light(0.0f, 1.0f);

glm::vec3 cosine_weighted_hemisphere_sample(const glm::vec3& normal, float& pdf) {
    float rand_1 = dist_light(rng_light);
    float rand_2 = dist_light(rng_light);

    float sqrt_rand_2 = sqrtf(rand_2);
    float phi = 2.0f * glm::pi<float>() * rand_1;
    float cos_theta = sqrt_rand_2;
    float sin_theta = sqrtf(fmax(0.0f, 1.0f - cos_theta * cos_theta));

    pdf = sqrt_rand_2 / glm::pi<float>();

    glm::vec3 random_dir_local_space = glm::vec3(cosf(phi) * sin_theta, sqrt_rand_2, sinf(phi) * sin_theta);
    return rotate_vector_around_normal(normal, random_dir_local_space);
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