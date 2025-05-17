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

glm::vec3 shade_normal(const HitInfo& hit, const SamplerResult& sample, World& scene) {
    // visualize uv coordinates;
    if (hit.t == 1E30f) {
        return sky_color(hit.r.direction());
    }

    return hit.triangle.normal(hit.uv);
}

glm::vec3 shade_debug(const HitInfo& hit, const SamplerResult& sample, World& scene) {
    // visualize uv coordinates;
    if (hit.t == 1E30f) {
		return sky_color(hit.r.direction());
    }

    // Albedo
	glm::vec3 fr = hit.mat_ptr->albedo(hit);

    return fr;
}


// Reference: https://momentsingraphics.de/ToyRenderer4RayTracing.html

static glm::vec3 shade(const HitInfo& hit, const SamplerResult& sample, World& scene) {
	// Sampled light direction
    const glm::vec3 L = sample.light_dir;

	// Normal of the intersection point
    const glm::vec3 N = hit.triangle.normal(hit.uv);

    // Normal of the light source
    const glm::vec3 Nl = sample.light.triangle.normal();

    // Point of intersection [x]
    const glm::vec3 I = hit.r.at(hit.t);

	// Distance between the light and the intersection point
    const float dist = glm::length(sample.light_point - I);

    // Emitted radiance from the light source towards x. For uniform area lights, it's constant: L0.
	const glm::vec3 Le = sample.light.c * sample.light.intensity;

    // Visibility term
    Ray shadow_ray = Ray(I + EPS * L, L);
    const float V = scene.is_occluded(shadow_ray, dist - 0.1f) ? 0.0f : 1.0f;

    // Early return
    if (V == 0.0f) {
        return glm::vec3(0.0f);
    }

	// BRDF term
	glm::vec3 fr = hit.mat_ptr->evaluate(hit, L);

    // Geometry term
    const float cos_theta = fabs(glm::dot(N, L));
    const float G = cos_theta;

    // Final contribution
    return Le * V * fr * G;
}

glm::vec3 shadeRIS(const HitInfo& hit, const SamplerResult& sample, World& scene) {
    // Ray has no intersection
    if (hit.t == 1E30f) {
        return sky_color(hit.r.direction());
    }

    // If the material emits light, return the emitted radiance directly
    if (hit.mat_ptr->emits_light()) {
        return hit.mat_ptr->albedo(hit);
    }

    glm::vec3 f = shade(hit, sample, scene);

    const float W = sample.W;

    return f * W;
}

glm::vec3 shadeUniform(const HitInfo& hit, const SamplerResult& sample, World& scene, RestirLightSampler& sampler) {
    // Ray has no intersection
    if (hit.t == 1E30f) {
        return sky_color(hit.r.direction());
    }

    // If the material emits light, return the emitted radiance directly
    if (hit.mat_ptr->emits_light()) {
        return hit.mat_ptr->albedo(hit);
    }

    // Sampled light direction
    const glm::vec3 L = sample.light_dir;

    // Normal of the light source
    const glm::vec3 Nl = sample.light.triangle.normal();

    // Point of intersection [x]
    const glm::vec3 I = hit.r.at(hit.t);

    // Distance between the light and the intersection point
    const float dist = glm::length(sample.light_point - I);

	glm::vec3 f = shade(hit, sample, scene);

    // Source PDF: converting from area to solid angle
    const float light_choose_pdf = 1.0f / sampler.num_lights;
    const float light_area_pdf = 1.0f / sample.light.area();
    const float dist2 = dist * dist;
	const float cos_theta_light = fabs(glm::dot(Nl, -L)); // Light angle
    const float source = light_choose_pdf * light_area_pdf * (dist2 / cos_theta_light); // dA → dOmega

	return f / source;
}
