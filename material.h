#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "ray.h"
#include "texture.h"
#include "hit_info.h"
#include "util.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

struct material {
	glm::vec3 c;
	float k_d = 1.0f; // Diffuse reflection coefficient
	float k_s = 0.0f; // Specular reflection coefficient
	float p = 1.0f; // Phong exponent
};

class Material {
public:
	virtual bool scatter(const Ray& r_In, const hit_info& hit, glm::vec3& attenuation, Ray& scattered) const = 0;
	virtual glm::vec3 evaluate(const hit_info& hit, const glm::vec3& wi) const = 0;
	virtual bool emits_light() const { return false; }
};

class Lambertian : public Material {
	Texture* albedo;
public:
	Lambertian(const glm::vec3& a) : albedo(new SolidColor(a)) {}
	Lambertian(Texture* a) : albedo(a) {}
	virtual bool scatter(const Ray& r_in, const hit_info& hit, glm::vec3& attenuation, Ray& scattered) const {
		glm::vec3 scatter_dir = random_in_hemisphere(hit.normal);

		if (near_zero(scatter_dir)) {
			scatter_dir = hit.normal;
		}

		scattered = Ray(hit.r.at(hit.t), scatter_dir);
		attenuation = albedo->value(0, 0, hit.r.at(hit.t));

		return true;
	}

	virtual glm::vec3 evaluate(const hit_info& hit, const glm::vec3& wi) const {
		if (glm::dot(hit.normal, wi) <= 0.0f) {
			return glm::vec3(0.0f);
		}

		return albedo->value(0, 0, hit.r.at(hit.t)) * glm::dot(hit.normal, wi) / glm::pi<float>();
	}
};

class Emissive : public Material {
	Texture* emit;
public:
	Emissive(const glm::vec3& a) : emit(new SolidColor(a)) {}
	Emissive(Texture* a) : emit(a) {}
	virtual bool scatter(const Ray& r_in, const hit_info& hit, glm::vec3& attenuation, Ray& scattered) const {
		return false;
	}
	virtual glm::vec3 evaluate(const hit_info& hit, const glm::vec3& wi) const {
		return emit->value(0, 0, hit.r.at(hit.t));
	}
	virtual bool emits_light() const { return true; }
};

#endif