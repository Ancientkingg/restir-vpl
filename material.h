#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "ray.h"
#include "texture.h"
#include "hit_info.h"
#include "util.h"

#include <glm/glm.hpp>

struct material {
	glm::vec3 c;
	float k_d = 1.0f; // Diffuse reflection coefficient
	float k_s = 0.0f; // Specular reflection coefficient
	float p = 1.0f; // Phong exponent
};

class Material {
public:
	virtual bool scatter(const Ray& r_In, const hit_info& hit, glm::vec3& attenuation, Ray& scattered) const = 0;
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
	}
};

class Metal : public Material {
	Texture* albedo;
	float fuzz;
public:
	Metal(const glm::vec3& a, float f) : albedo(new SolidColor(a)), fuzz(f < 1 ? f : 1) {}
	Metal(Texture* a, float f) : albedo(a), fuzz(f < 1 ? f : 1) {}
	virtual bool scatter(const Ray& r_in, const hit_info& hit, glm::vec3& attenuation, Ray& scattered) const {
		glm::vec3 reflected = reflect(glm::normalize(r_in.direction()), hit.normal);
		scattered = Ray(hit.r.at(hit.t), reflected + fuzz * random_in_unit_sphere());
		attenuation = albedo->value(0, 0, hit.r.at(hit.t));
		return (glm::dot(scattered.direction(), hit.normal) > 0);
	}
};

#endif