#include "material.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <random>

#include "ray.hpp"
#include "texture.hpp"
#include "hit_info.hpp"
#include "util.hpp"
#include "light.hpp"

thread_local std::mt19937 rngFloat(std::random_device{}());
std::uniform_real_distribution<float> distFloat(0.0f, 1.0f);

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
Lambertian::Lambertian(std::shared_ptr<Texture> a) : _albedo(a) {}
bool Lambertian::scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered, float& pdf) const {
	glm::vec3 scatter_dir = cosine_weighted_hemisphere_sample(hit.triangle.normal(hit.uv), pdf);

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

	attenuation = _albedo->value(texcoords.x, texcoords.y, hit.r.at(hit.t)) / glm::pi<float>();

	return true;
}

glm::vec3 Lambertian::evaluate(const HitInfo& hit, const glm::vec3& wi) const {
	glm::vec2 texcoords = calculate_texcoords(
		hit.triangle.v0.texcoord,
		hit.triangle.v1.texcoord,
		hit.triangle.v2.texcoord,
		hit.uv
	);

	return _albedo->value(texcoords.x, texcoords.y, hit.r.at(hit.t)) / glm::pi<float>();
}

glm::vec3 Lambertian::sample_direction(
	const glm::vec3& wi,
	const glm::vec3& normal,
	float& pdf
) const {
	// Cosine-weighted hemisphere sampling
	float u = distFloat(rngFloat);
	float v = distFloat(rngFloat);
	float r = sqrt(u);
	float theta = 2.0f * glm::pi<float>() * v;
	// local coordinates
	float x = r * cos(theta);
	float y = r * sin(theta);
	float z = sqrt(1.0f - u);
	// build tangent space basis
	glm::vec3 tangent = fabs(normal.x) > 0.1f ?
		glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0))) :
		glm::normalize(glm::cross(normal, glm::vec3(1, 0, 0)));
	glm::vec3 bitangent = glm::cross(normal, tangent);
	// transform to world
	glm::vec3 sample = x * tangent + y * bitangent + z * normal;

	pdf = z / glm::pi<float>(); // cos(theta) / pi
	return glm::normalize(sample);
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

Emissive::Emissive(std::shared_ptr<Texture> a) : emit(a) {}

bool Emissive::scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered, float& pdf) const {
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

glm::vec3 Emissive::sample_direction(
	const glm::vec3& wi,
	const glm::vec3& normal,
	float& pdf
) const {
	// Emissive materials do not scatter, so we return a zero vector
	pdf = 0.0f;
	return glm::vec3(0.0f);
}

glm::vec3 Emissive::albedo(const HitInfo& hit) const {
	glm::vec2 texcoords = calculate_texcoords(
		hit.triangle.v0.texcoord,
		hit.triangle.v1.texcoord,
		hit.triangle.v2.texcoord,
		hit.uv
	);

	return emit->value(texcoords.x, texcoords.y, hit.r.at(hit.t));
}
