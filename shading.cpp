#include "shading.hpp"

#include <glm/glm.hpp>

#include "hit_info.hpp"
#include "restir.hpp"
#include "world.hpp"
#include "ray.hpp"

#define RED glm::vec3(1.0f,0.0f,0.0f)
#define GREEN glm::vec3(0.0f,1.0f,0.0f)
#define BLUE glm::vec3(0.0f,0.0f,1.0f)
#define PURPLE glm::vec3(0.5f,0.0f,0.5f)
#define BLACK glm::vec3(0.0f,0.0f,0.0f)
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

    // loop through all vpls and find out if the hit point is within a certain radius r of any of the vpls
	glm::vec3 hit_point = hit.r.at(hit.t);

    glm::vec3 closest_vpl = scene.vpl_cloud.find_closest(hit_point);
    float dist = glm::length(hit_point - closest_vpl);
    if (dist < 0.05f) {
        return GREEN;
    }

    glm::vec3 closest_pl = scene.point_light_cloud.find_closest(hit_point);
    dist = glm::length(hit_point - closest_pl);
    if (dist < 0.05f) {
        return BLUE;
    }

    // Albedo
    auto material = hit.mat_ptr.lock();
	glm::vec3 fr = material->albedo(hit);

	// Cosine of the angle between the surface normal and the ray direction
	const glm::vec3 N = hit.triangle.normal(hit.uv);
	const float cos_theta = fmax(glm::dot(N, -hit.r.direction()), 0.0f);

    return fr * cos_theta;
}


// Reference: https://momentsingraphics.de/ToyRenderer4RayTracing.html

static glm::vec3 shade(const HitInfo& hit, const SamplerResult& sample, World& scene) {
	if (sample.light.expired()) {
		return glm::vec3(0.0f);
	}

	auto light = sample.light.lock();

	// Sampled light direction
    const glm::vec3 L = sample.light_dir;

	// Normal of the intersection point
    const glm::vec3 N = hit.triangle.normal(hit.uv);

    // Normal of the light source
    const glm::vec3 Nl = light->normal(sample.light_point);

    // Point of intersection [x]
    const glm::vec3 I = hit.r.at(hit.t);

	// Distance between the light and the intersection point
    const float dist = glm::length(sample.light_point - I);

    // Emitted radiance from the light source towards x. For uniform area lights, it's constant: L0.
	const glm::vec3 Le = light->c * light->intensity;

    // Visibility term
    Ray shadow_ray = Ray(I + EPS * L, L);
    // 0.005f
    const float V = scene.is_occluded(shadow_ray, dist - 0.1f) ? 0.0f : 1.0f;

    // Early return
    if (V == 0.0f) {
        return glm::vec3(0.0f);
    }

	// BRDF term
	auto material = hit.mat_ptr.lock();
	glm::vec3 fr = material->evaluate(hit, L);

    // Geometry term
    const float cos_theta = fmax(glm::dot(N, L), 0.0f);
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
	auto material = hit.mat_ptr.lock();
    if (material->emits_light()) {
        return material->albedo(hit);
    }

    if (sample.light.expired()) {
        return BLACK;
        return rand() % 2 == 0 ? GREEN : BLACK; // Return a random color if no light is sampled
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
	auto material = hit.mat_ptr.lock();
    if (material->emits_light()) {
        return material->albedo(hit);
    }

    if (sample.light.expired()) {
        return BLACK;
        return rand() % 2 == 0 ? GREEN : BLACK; // Return a random color if no light is sampled
    }

    auto light = sample.light.lock();

    // Sampled light direction
    const glm::vec3 L = sample.light_dir;

    // Normal of the light source
    const glm::vec3 Nl = light->normal(sample.light_point);

    // Point of intersection [x]
    const glm::vec3 I = hit.r.at(hit.t);

    // Distance between the light and the intersection point
    const float dist = glm::length(sample.light_point - I);

	glm::vec3 f = shade(hit, sample, scene);

    // Source PDF: converting from area to solid angle
    const float light_choose_pdf = 1.0f / sampler.num_lights();
    const float light_area_pdf = 1.0f / light->area();
    const float dist2 = dist * dist;
	const float cos_theta_light = fmax(glm::dot(Nl, -L), 0.0f); // Light angle
    const float source = light_choose_pdf * light_area_pdf * (dist2 / cos_theta_light); // dA → dOmega

    if (cos_theta_light == 0.0f) {
		// If the cosine of the angle is zero, we cannot divide by zero.
		return glm::vec3(0.0f);
    }

	return f / source;
}
