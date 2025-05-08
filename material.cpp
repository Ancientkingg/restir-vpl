#include "material.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "ray.hpp"
#include "texture.hpp"
#include "hit_info.hpp"
#include "util.hpp"

bool Material::emits_light() const { return false; }

glm::vec3 Material::albedo(const HitInfo& hit) const { return glm::vec3(1.0f); }


Lambertian::Lambertian(const glm::vec3& a) : _albedo(new SolidColor(a)) {}
Lambertian::Lambertian(Texture* a) : _albedo(a) {}
bool Lambertian::scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered) const {
	glm::vec3 scatter_dir = random_in_hemisphere(hit.normal);

	if (near_zero(scatter_dir)) {
		scatter_dir = hit.normal;
	}

	scattered = Ray(hit.r.at(hit.t), scatter_dir);
	attenuation = _albedo->value(hit.uv.x, hit.uv.y, hit.r.at(hit.t));

	return true;
}

glm::vec3 Lambertian::evaluate(const HitInfo& hit, const glm::vec3& wi) const {
	if (glm::dot(hit.normal, wi) <= 0.0f) {
		return glm::vec3(0.0f);
	}

	return _albedo->value(hit.uv.x, hit.uv.y, hit.r.at(hit.t)) * glm::dot(hit.normal, wi) / glm::pi<float>();
}

glm::vec3 Lambertian::albedo(const HitInfo& hit) const {
	return _albedo->value(hit.uv.x, hit.uv.y, hit.r.at(hit.t));
}


Emissive::Emissive(const glm::vec3& a) : emit(new SolidColor(a)) {}

Emissive::Emissive(Texture* a) : emit(a) {}

bool Emissive::scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered) const {
	return false;
}
glm::vec3 Emissive::evaluate(const HitInfo& hit, const glm::vec3& wi) const {
	return emit->value(hit.uv.x, hit.uv.y, hit.r.at(hit.t));
}
bool Emissive::emits_light() const { return true; }

glm::vec3 Emissive::albedo(const HitInfo& hit) const {
	return emit->value(hit.uv.x, hit.uv.y, hit.r.at(hit.t));
}
