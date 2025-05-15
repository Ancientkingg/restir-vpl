#include "material.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "ray.hpp"
#include "texture.hpp"
#include "hit_info.hpp"
#include "util.hpp"

bool Material::emits_light() const { return false; }

glm::vec3 Material::albedo(const HitInfo& hit) const { return glm::vec3(1.0f); }

static glm::vec2 calculate_texcoords(const glm::vec2& texcoord0, const glm::vec2& texcoord1, const glm::vec2& texcoord2, const glm::vec2& uv) {
	const float u = uv.x;
	const float v = uv.y;
	const float w = 1.0f - u - v;

	const glm::vec2 texcoords = texcoord0 * u +
		texcoord1 * v +
		texcoord2 * w;

	return texcoords;
}

Lambertian::Lambertian(const glm::vec3& a) : _albedo(new SolidColor(a)) {}
Lambertian::Lambertian(Texture* a) : _albedo(a) {}
bool Lambertian::scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered) const {
	glm::vec3 scatter_dir = random_in_hemisphere(hit.triangle.normal(hit.uv));

	if (near_zero(scatter_dir)) {
		scatter_dir = hit.triangle.normal(hit.uv);
	}

	scattered = Ray(hit.r.at(hit.t), scatter_dir);

	glm::vec2 texcoords = calculate_texcoords(
		hit.triangle.v0.texcoord,
		hit.triangle.v1.texcoord,
		hit.triangle.v2.texcoord,
		hit.uv
	);

	attenuation = _albedo->value(texcoords.x, texcoords.y, hit.r.at(hit.t));

	return true;
}

glm::vec3 Lambertian::evaluate(const HitInfo& hit, const glm::vec3& wi) const {
	const glm::vec3 N = hit.triangle.normal(hit.uv);
	if (glm::dot(N, wi) <= 0.0f) {
		return glm::vec3(0.0f);
	}

	glm::vec2 texcoords = calculate_texcoords(
		hit.triangle.v0.texcoord,
		hit.triangle.v1.texcoord,
		hit.triangle.v2.texcoord,
		hit.uv
	);

	return _albedo->value(texcoords.x, texcoords.y, hit.r.at(hit.t)) / glm::pi<float>();
}

glm::vec3 Lambertian::albedo(const HitInfo& hit) const {
	glm::vec2 texcoords = calculate_texcoords(
		hit.triangle.v0.texcoord,
		hit.triangle.v1.texcoord,
		hit.triangle.v2.texcoord,
		hit.uv
	);

	return _albedo->value(texcoords.x, texcoords.y, hit.r.at(hit.t));
}


Emissive::Emissive(const glm::vec3& a) : emit(new SolidColor(a)) {}

Emissive::Emissive(Texture* a) : emit(a) {}

bool Emissive::scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered) const {
	return false;
}
glm::vec3 Emissive::evaluate(const HitInfo& hit, const glm::vec3& wi) const {
	glm::vec2 texcoords = calculate_texcoords(
		hit.triangle.v0.texcoord,
		hit.triangle.v1.texcoord,
		hit.triangle.v2.texcoord,
		hit.uv
	);

	return emit->value(texcoords.x, texcoords.y, hit.r.at(hit.t));
}
bool Emissive::emits_light() const { return true; }

glm::vec3 Emissive::albedo(const HitInfo& hit) const {
	glm::vec2 texcoords = calculate_texcoords(
		hit.triangle.v0.texcoord,
		hit.triangle.v1.texcoord,
		hit.triangle.v2.texcoord,
		hit.uv
	);

	return emit->value(texcoords.x, texcoords.y, hit.r.at(hit.t));
}
