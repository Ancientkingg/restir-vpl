#include "shading.hpp"

#include <glm/glm.hpp>

#include "hit_info.hpp"
#include "light_sampler.hpp"
#include "world.hpp"
#include "ray.hpp"

#define RED glm::vec3(1.0f,0.0f,0.0f)
#define GREEN glm::vec3(0.0f,1.0f,0.0f)
#define BLUE glm::vec3(0.0f,0.0f,1.0f)
#define EPS 0.001f
#define rougheq(x,y) (fabs(x-y) < EPS)

glm::vec3 sky_color(const glm::vec3& direction) {
    float t = 0.5f * (direction.y + 1.0f);
    glm::vec3 top = glm::vec3(0.5f, 0.7f, 1.0f);    // Sky blue
    glm::vec3 bottom = glm::vec3(1.0f);            // Horizon white
    return (1.0f - t) * bottom + t * top;
}

glm::vec3 shade_normal(const HitInfo& hit, const SamplerResult& sample, float pdf, World& scene) {
    return hit.normal;
}

glm::vec3 shade_debug(const HitInfo& hit, const SamplerResult& sample, float pdf, World& scene) {
    float x = sample.light_dir.y;

    if (x < 0.0f) {
        return glm::vec3(-x, 0.0f, 0.0f); // Red
    }
    else {
        return glm::vec3(0.0f, x, 0.0f); // Green
    }
}

glm::vec3 shade(const HitInfo& hit, const SamplerResult& sample, float pdf, World& scene) {
    glm::vec3 L = sample.light_dir;
    glm::vec3 N = hit.normal;

    // Ray has no intersection
    if (hit.t == 1E30f) {
        return sky_color(hit.r.direction());
    }

    // If the material emits light, return the emitted radiance directly
    if (hit.mat_ptr->emits_light()) {
        return hit.mat_ptr->albedo(hit);
    }

    glm::vec3 I = hit.r.at(hit.t);
    float dist = glm::length(sample.light_point - I);

    // Evaluate BRDF at the hit point
    glm::vec3 brdf = hit.mat_ptr->evaluate(hit, sample.light_dir);

    glm::vec3 direct = glm::vec3(0.0f);
    if (Ray shadow_ray = Ray(I + EPS * L, L); !scene.is_occluded(shadow_ray, dist - EPS * 5000.0f)) {
        direct = brdf;
    }

    glm::vec3 ambient_lighting = sample.light.c; // Ambient light color
    float ambient_cos_theta = fabs(glm::dot(L, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 ambient = ambient_lighting * ambient_cos_theta * 1000.0f * sample.light.intensity * (1.0f / (dist * dist));

    // Final contribution
    return direct * ambient;
}